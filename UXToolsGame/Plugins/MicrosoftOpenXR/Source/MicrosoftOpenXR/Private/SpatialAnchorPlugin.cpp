// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "SpatialAnchorPlugin.h"
#include "OpenXRCore.h"
#include "ARPin.h"

#include "GameDelegates.h"

#if WINRT_ANCHOR_STORE_AVAILABLE 

using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Foundation;

#endif

namespace MicrosoftOpenXR
{
	void FSpatialAnchorPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

		FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FSpatialAnchorPlugin::OnEndPlay);
	}

	void FSpatialAnchorPlugin::Unregister()
	{
#if WINRT_ANCHOR_STORE_AVAILABLE 
		{
			std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
			if (m_spatialAnchorStoreAsyncOperation != nullptr && m_spatialAnchorStoreAsyncOperation.Status() != winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				m_spatialAnchorStoreAsyncOperation.Cancel();
			}
			m_spatialAnchorStore = nullptr;
		}
#endif

		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);

		FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
	}

	void FSpatialAnchorPlugin::OnEndPlay()
	{
		if (SpatialAnchorStoreMSFT != XR_NULL_HANDLE)
		{
			XR_ENSURE_MSFT(xrDestroySpatialAnchorStoreConnectionMSFT(SpatialAnchorStoreMSFT));
			SpatialAnchorStoreMSFT = XR_NULL_HANDLE;
		}
	}

	bool FSpatialAnchorPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME);
		return true;
	}

	
	bool FSpatialAnchorPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) 
	{
#if WINRT_ANCHOR_STORE_AVAILABLE
		OutExtensions.Add(XR_MSFT_PERCEPTION_ANCHOR_INTEROP_EXTENSION_NAME);
#endif

		OutExtensions.Add(XR_MSFT_SPATIAL_ANCHOR_PERSISTENCE_EXTENSION_NAME);

		return true;
	}


	const void* FSpatialAnchorPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) 
	{
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorMSFT", (PFN_xrVoidFunction*) &xrCreateSpatialAnchorMSFT));
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorSpaceMSFT",(PFN_xrVoidFunction*) &xrCreateSpatialAnchorSpaceMSFT));
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrDestroySpatialAnchorMSFT", (PFN_xrVoidFunction*) &xrDestroySpatialAnchorMSFT));

		bIsAnchorPersistenceExtensionSupported =
			IOpenXRHMDModule::Get().IsExtensionEnabled(XR_MSFT_SPATIAL_ANCHOR_PERSISTENCE_EXTENSION_NAME);

		if (bIsAnchorPersistenceExtensionSupported)
		{
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorStoreConnectionMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialAnchorStoreConnectionMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrDestroySpatialAnchorStoreConnectionMSFT", (PFN_xrVoidFunction*)&xrDestroySpatialAnchorStoreConnectionMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrPersistSpatialAnchorMSFT", (PFN_xrVoidFunction*)&xrPersistSpatialAnchorMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrEnumeratePersistedSpatialAnchorNamesMSFT", (PFN_xrVoidFunction*)&xrEnumeratePersistedSpatialAnchorNamesMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorFromPersistedNameMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialAnchorFromPersistedNameMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrUnpersistSpatialAnchorMSFT", (PFN_xrVoidFunction*)&xrUnpersistSpatialAnchorMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrClearSpatialAnchorStoreMSFT", (PFN_xrVoidFunction*)&xrClearSpatialAnchorStoreMSFT));
		}

#if WINRT_ANCHOR_STORE_AVAILABLE 
		bIsPerceptionAnchorInteropExtensionSupported =
			IOpenXRHMDModule::Get().IsExtensionEnabled(XR_MSFT_PERCEPTION_ANCHOR_INTEROP_EXTENSION_NAME);
		
		if (bIsPerceptionAnchorInteropExtensionSupported)
		{
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorFromPerceptionAnchorMSFT", (PFN_xrVoidFunction*) &xrCreateSpatialAnchorFromPerceptionAnchorMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrTryGetPerceptionAnchorFromSpatialAnchorMSFT", (PFN_xrVoidFunction*) &xrTryGetPerceptionAnchorFromSpatialAnchorMSFT));

			{
				std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
				m_spatialAnchorStoreAsyncOperation = SpatialAnchorManager::RequestStoreAsync();
				m_spatialAnchorStoreAsyncOperation.Completed([this](IAsyncOperation<SpatialAnchorStore> asyncOperation, AsyncStatus status) 
				{
					std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
					if (asyncOperation.Status() == AsyncStatus::Completed)
					{
						m_spatialAnchorStore = asyncOperation.GetResults();
					}

					m_spatialAnchorStoreAsyncOperation = nullptr;
				});
			}
		}
#endif
		return InNext;
	}

	const void* FSpatialAnchorPlugin::OnBeginSession(XrSession InSession, const void* InNext)
	{
		Session = InSession;

		if (bIsAnchorPersistenceExtensionSupported)
		{
			XR_ENSURE_MSFT(xrCreateSpatialAnchorStoreConnectionMSFT(InSession, &SpatialAnchorStoreMSFT));
		}

		return InNext;
	}

	IOpenXRCustomAnchorSupport* FSpatialAnchorPlugin::GetCustomAnchorSupport()
	{
		return this;
	}


	bool FSpatialAnchorPlugin::OnPinComponent(class UARPin* NewPin, XrSession InSession, XrSpace TrackingSpace, XrTime DisplayTime, float worldToMeterScale) 
	{
		XrResult result;

		XrSpatialAnchorCreateInfoMSFT AnchorCreateDesc = {};
		AnchorCreateDesc.type = XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_MSFT;
		AnchorCreateDesc.next = nullptr;
		AnchorCreateDesc.pose = ToXrPose(NewPin->GetLocalToTrackingTransform(), worldToMeterScale);
		AnchorCreateDesc.space = TrackingSpace;
		AnchorCreateDesc.time = DisplayTime;

		XrSpatialAnchorMSFT AnchorId = {};
		result = xrCreateSpatialAnchorMSFT(InSession, &AnchorCreateDesc, &AnchorId);

		if (XR_FAILED(result))
		{
			return false;
		}

		XrSpatialAnchorSpaceCreateInfoMSFT AnchorSpaceCreateDesc = {};
		AnchorSpaceCreateDesc.type = XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT;
		AnchorSpaceCreateDesc.next = nullptr;
		AnchorSpaceCreateDesc.poseInAnchorSpace = ToXrPose(FTransform::Identity);
		AnchorSpaceCreateDesc.anchor = AnchorId;

		XrSpace AnchorSpace = {};
		result = xrCreateSpatialAnchorSpaceMSFT(InSession, &AnchorSpaceCreateDesc, &AnchorSpace);

		if (XR_FAILED(result))
		{
			xrDestroySpatialAnchorMSFT(AnchorId);
			return false;
		}

		SAnchorMSFT* AnchorMSFT = new SAnchorMSFT;
		AnchorMSFT->Anchor = AnchorId;
		AnchorMSFT->Space = AnchorSpace;

		NewPin->SetNativeResource(reinterpret_cast<void*>(AnchorMSFT));

		return true;
	}

	void FSpatialAnchorPlugin::OnRemovePin(class UARPin* Pin) 
	{
		if (void* nativeResource = Pin->GetNativeResource())
		{
			SAnchorMSFT* AnchorMSFT = reinterpret_cast<SAnchorMSFT*>(nativeResource);
			xrDestroySpatialAnchorMSFT(AnchorMSFT->Anchor);
			xrDestroySpace(AnchorMSFT->Space);
			delete AnchorMSFT;
		}
	}

	void FSpatialAnchorPlugin::OnUpdatePin(class UARPin* Pin, XrSession InSession, XrSpace TrackingSpace, XrTime DisplayTime, float worldToMeterScale)
	{
		if (void* nativeResource = Pin->GetNativeResource())
		{
			SAnchorMSFT* AnchorMSFT = reinterpret_cast<SAnchorMSFT*>(nativeResource);
			XrResult result;
			XrSpaceLocation SpaceLocation = {};
			SpaceLocation.type = XR_TYPE_SPACE_LOCATION;
			SpaceLocation.next = nullptr;

			result = xrLocateSpace(AnchorMSFT->Space, TrackingSpace, DisplayTime, &SpaceLocation);

			const XrSpaceLocationFlags ValidFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
			if (XR_SUCCEEDED(result) && ((SpaceLocation.locationFlags & ValidFlags) == ValidFlags))
			{
				FTransform Transform = ToFTransform(SpaceLocation.pose, worldToMeterScale);

				Pin->OnTransformUpdated(Transform);
				Pin->OnTrackingStateChanged(EARTrackingState::Tracking);
			}
			else
			{
				Pin->OnTrackingStateChanged(EARTrackingState::NotTracking);
			}
		}
	}

	bool FSpatialAnchorPlugin::IsLocalPinSaveSupported() const 
	{
		return bIsPerceptionAnchorInteropExtensionSupported || bIsAnchorPersistenceExtensionSupported;
	}

	bool FSpatialAnchorPlugin::IsAnchorStoreReady()
	{
		// ArePinsReadyToLoad returns true if the anchor store is non-null.
		// The name can be confusing, since we need to verify the anchor store is non-null when also saving anchors.
		return ArePinsReadyToLoad();
	}

	bool FSpatialAnchorPlugin::ArePinsReadyToLoad() 
	{
		if (bIsAnchorPersistenceExtensionSupported)
		{
			return SpatialAnchorStoreMSFT != XR_NULL_HANDLE;
		}

#if WINRT_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		return m_spatialAnchorStore != nullptr;
#else
		return false;
#endif
	}

	void FSpatialAnchorPlugin::LoadARPins(XrSession InSession, TFunction<UARPin*(FName)> OnCreatePin) 
	{
		if (!IsAnchorStoreReady())
		{
			return;
		}

		if (bIsAnchorPersistenceExtensionSupported)
		{
			uint32_t AnchorCount = 0;
			XR_ENSURE_MSFT(xrEnumeratePersistedSpatialAnchorNamesMSFT(SpatialAnchorStoreMSFT, 0, &AnchorCount, nullptr));

			std::vector<XrSpatialAnchorPersistenceNameMSFT> AnchorNames;
			AnchorNames.resize(AnchorCount);

			XR_ENSURE_MSFT(xrEnumeratePersistedSpatialAnchorNamesMSFT(
				SpatialAnchorStoreMSFT, AnchorCount, &AnchorCount, AnchorNames.data()));

			for (const XrSpatialAnchorPersistenceNameMSFT& AnchorName : AnchorNames)
			{
				auto NewPin = OnCreatePin(FName(AnchorName.name));

				if (NewPin == nullptr)
				{
					//the anchor is already loaded
					continue;
				}

				XrSpatialAnchorFromPersistedAnchorCreateInfoMSFT CreateSpatialAnchorInfo{
					XR_TYPE_SPATIAL_ANCHOR_FROM_PERSISTED_ANCHOR_CREATE_INFO_MSFT };
				CreateSpatialAnchorInfo.spatialAnchorStore = SpatialAnchorStoreMSFT;
				CreateSpatialAnchorInfo.spatialAnchorPersistenceName = AnchorName;

				XrSpatialAnchorMSFT SpatialAnchor;
				XR_ENSURE_MSFT(xrCreateSpatialAnchorFromPersistedNameMSFT(
					InSession, &CreateSpatialAnchorInfo, &SpatialAnchor));

				XrSpatialAnchorSpaceCreateInfoMSFT AnchorSpaceCreateInfo{ 
					XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT };
				AnchorSpaceCreateInfo.poseInAnchorSpace = ToXrPose(FTransform::Identity);
				AnchorSpaceCreateInfo.anchor = SpatialAnchor;

				XrSpace AnchorSpace = {};
				if (XR_FAILED(xrCreateSpatialAnchorSpaceMSFT(InSession, &AnchorSpaceCreateInfo, &AnchorSpace)))
				{
					xrDestroySpatialAnchorMSFT(SpatialAnchor);
					continue;
				}

				SAnchorMSFT* AnchorMSFT = new SAnchorMSFT;
				AnchorMSFT->Anchor = SpatialAnchor;
				AnchorMSFT->Space = AnchorSpace;

				NewPin->SetNativeResource(reinterpret_cast<void*>(AnchorMSFT));
			}

			return;
		}

#if WINRT_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return; }

		for(auto p : m_spatialAnchorStore.GetAllSavedAnchors())
		{
			XrResult result;

			auto name = p.Key();
			auto wmrAnchor = p.Value();

			auto NewPin = OnCreatePin(FName(name.c_str()));

			if (NewPin == nullptr)
			{
				//the anchor is already loaded
				continue;
			}

			XrSpatialAnchorMSFT AnchorId = {};
			result = xrCreateSpatialAnchorFromPerceptionAnchorMSFT(InSession, winrt::get_unknown(wmrAnchor), &AnchorId);

			if (XR_FAILED(result))
			{
				continue;
			}

			XrSpatialAnchorSpaceCreateInfoMSFT AnchorSpaceCreateDesc = {};
			AnchorSpaceCreateDesc.type = XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT;
			AnchorSpaceCreateDesc.next = nullptr;
			AnchorSpaceCreateDesc.poseInAnchorSpace = ToXrPose(FTransform::Identity);
			AnchorSpaceCreateDesc.anchor = AnchorId;

			XrSpace AnchorSpace = {};
			result = xrCreateSpatialAnchorSpaceMSFT(InSession, &AnchorSpaceCreateDesc, &AnchorSpace);

			if (XR_FAILED(result))
			{
				xrDestroySpatialAnchorMSFT(AnchorId);
				continue;
			}

			SAnchorMSFT* AnchorMSFT = new SAnchorMSFT;
			AnchorMSFT->Anchor = AnchorId;
			AnchorMSFT->Space = AnchorSpace;

			NewPin->SetNativeResource(reinterpret_cast<void*>(AnchorMSFT));
		}
#endif
	}

	bool FSpatialAnchorPlugin::SaveARPin(XrSession InSession, FName InName, UARPin* Pin) 
	{
		if (!IsAnchorStoreReady())
		{
			return false;
		}

		if (bIsAnchorPersistenceExtensionSupported)
		{
			const FString SaveId = InName.ToString().ToLower();

			void* nativeResource = Pin->GetNativeResource();
			if (nativeResource == nullptr) { return false; }

			SAnchorMSFT* AnchorMSFT = reinterpret_cast<SAnchorMSFT*>(nativeResource);

			XrSpatialAnchorPersistenceInfoMSFT PersistenceInfo{ XR_TYPE_SPATIAL_ANCHOR_PERSISTENCE_INFO_MSFT };

			FTCHARToUTF8 UTF8ConvertedString(*SaveId);
			// Length() returns size without null terminator.
			// Anchor name is valid up to XR_MAX_SPATIAL_ANCHOR_NAME_SIZE_MSFT - 1 to ensure room for a null terminator.
			if (UTF8ConvertedString.Length() >= XR_MAX_SPATIAL_ANCHOR_NAME_SIZE_MSFT)
			{
				UE_LOG(LogHMD, Warning, TEXT("Pin name is too long.  ARPin will not be saved."));
				return false;
			}

			// Length + 1 to ensure null terminator is included
			FPlatformString::Strncpy(PersistenceInfo.spatialAnchorPersistenceName.name, UTF8ConvertedString.Get(), UTF8ConvertedString.Length() + 1);
			PersistenceInfo.spatialAnchor = AnchorMSFT->Anchor;

			XrResult result = xrPersistSpatialAnchorMSFT(SpatialAnchorStoreMSFT, &PersistenceInfo);
			return XR_SUCCEEDED(result);
		}

#if WINRT_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return false; }

		void* nativeResource = Pin->GetNativeResource();
		if (nativeResource == nullptr) { return false; }

		SAnchorMSFT* AnchorMSFT = reinterpret_cast<SAnchorMSFT*>(nativeResource);
		XrResult result;

		SpatialAnchor wmrAnchor = nullptr;
		result = xrTryGetPerceptionAnchorFromSpatialAnchorMSFT(InSession, AnchorMSFT->Anchor, reinterpret_cast<::IUnknown**>(winrt::put_abi(wmrAnchor)));
		if (XR_FAILED(result))
		{
			return false;
		}

		const FString SaveId = InName.ToString().ToLower();
		return m_spatialAnchorStore.TrySave(*SaveId, wmrAnchor);
#else
		return false;
#endif
	}

	void FSpatialAnchorPlugin::RemoveSavedARPin(XrSession InSession, FName InName)
	{
		if (!IsAnchorStoreReady())
		{
			return;
		}

		if (bIsAnchorPersistenceExtensionSupported)
		{
			const FString SaveId = InName.ToString().ToLower();
			XrSpatialAnchorPersistenceNameMSFT SpatialAnchorPersistenceName;

			FTCHARToUTF8 UTF8ConvertedString(*SaveId);
			// Length() returns size without null terminator.
			// Anchor name is valid up to XR_MAX_SPATIAL_ANCHOR_NAME_SIZE_MSFT - 1 to ensure room for a null terminator.
			if (UTF8ConvertedString.Length() >= XR_MAX_SPATIAL_ANCHOR_NAME_SIZE_MSFT)
			{
				UE_LOG(LogHMD, Warning, TEXT("Pin name is too long.  Anchor will not be removed."));
				return;
			}

			// Length + 1 to ensure null terminator is included
			FPlatformString::Strncpy(SpatialAnchorPersistenceName.name, UTF8ConvertedString.Get(), UTF8ConvertedString.Length() + 1);

			xrUnpersistSpatialAnchorMSFT(SpatialAnchorStoreMSFT, &SpatialAnchorPersistenceName);
			return;
		}

#if WINRT_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return; }

		const FString SaveId = InName.ToString().ToLower();
		m_spatialAnchorStore.Remove(*SaveId);
#endif
	}

	void FSpatialAnchorPlugin::RemoveAllSavedARPins(XrSession InSession)
	{
		if (!IsAnchorStoreReady())
		{
			return;
		}

		if (bIsAnchorPersistenceExtensionSupported)
		{
			xrClearSpatialAnchorStoreMSFT(SpatialAnchorStoreMSFT);
			return;
		}

#if WINRT_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return; }

		m_spatialAnchorStore.Clear();
#endif
	}

	bool FSpatialAnchorPlugin::GetPerceptionAnchorFromOpenXRAnchor(XrSpatialAnchorMSFT AnchorId, ::IUnknown** OutPerceptionAnchor)
	{
#if WINRT_ANCHOR_STORE_AVAILABLE 
		if (!bIsPerceptionAnchorInteropExtensionSupported)
		{
			UE_LOG(LogHMD, Warning, TEXT("Attempting to get perception anchor, but local anchor store is not supported."));
			return false;
		}

		XrResult result;
		result = xrTryGetPerceptionAnchorFromSpatialAnchorMSFT(Session, AnchorId, reinterpret_cast<::IUnknown**>(winrt::put_abi(*OutPerceptionAnchor)));
		if (XR_FAILED(result))
		{
			UE_LOG(LogHMD, Warning, TEXT("xrTryGetPerceptionAnchorFromSpatialAnchorMSFT failed.  Ignoring."));
			return false;
		}

		return true;
#else
		return false;
#endif
	}

	bool FSpatialAnchorPlugin::StorePerceptionAnchor(const FString& InPinId, ::IUnknown* InPerceptionAnchor)
	{
#if WINRT_ANCHOR_STORE_AVAILABLE 
		if (!IsAnchorStoreReady())
		{
			return false;
		}

		if (bIsAnchorPersistenceExtensionSupported)
		{
			XrSpatialAnchorMSFT Anchor = {};
			if (XR_FAILED(xrCreateSpatialAnchorFromPerceptionAnchorMSFT(Session, InPerceptionAnchor, &Anchor)))
			{
				return false;
			}

			SAnchorMSFT* AnchorMSFT = new SAnchorMSFT;
			AnchorMSFT->Anchor = Anchor;

			XrSpatialAnchorPersistenceInfoMSFT PersistenceInfo{ XR_TYPE_SPATIAL_ANCHOR_PERSISTENCE_INFO_MSFT };

			FTCHARToUTF8 UTF8ConvertedString(*InPinId.ToLower());
			
			// Length() returns size without null terminator.
			// Anchor name is valid up to XR_MAX_SPATIAL_ANCHOR_NAME_SIZE_MSFT - 1 to ensure room for a null terminator.
			if (UTF8ConvertedString.Length() >= XR_MAX_SPATIAL_ANCHOR_NAME_SIZE_MSFT)
			{
				UE_LOG(LogHMD, Warning, TEXT("Pin name is too long.  Perception anchor will not be stored."));
				return false;
			}

			// Length + 1 to ensure null terminator is included
			FPlatformString::Strncpy(PersistenceInfo.spatialAnchorPersistenceName.name, UTF8ConvertedString.Get(), UTF8ConvertedString.Length() + 1);

			PersistenceInfo.spatialAnchor = AnchorMSFT->Anchor;

			return XR_SUCCEEDED(xrPersistSpatialAnchorMSFT(SpatialAnchorStoreMSFT, &PersistenceInfo));
		}

		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) 
		{ 
			UE_LOG(LogHMD, Warning, TEXT("Attempting to store perception anchor, but local anchor store is not supported."));
			return false; 
		}

		SpatialAnchor localAnchor = nullptr;
		if (FAILED(InPerceptionAnchor->QueryInterface(winrt::guid_of<SpatialAnchor>(), winrt::put_abi(localAnchor))))
		{
			UE_LOG(LogHMD, Warning, TEXT("StorePerceptionAnchor failed to get SpatialAnchor."));
			return false;
		}

		return m_spatialAnchorStore.TrySave(*InPinId.ToLower(), localAnchor);
#else
		return false;
#endif
	}

}	 // namespace MicrosoftOpenXR

