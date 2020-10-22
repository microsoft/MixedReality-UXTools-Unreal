#include "SpatialAnchorPlugin.h"
#include "OpenXRCore.h"
#include "ARPin.h"

#if HL_ANCHOR_STORE_AVAILABLE 

using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Foundation;

#pragma warning(push)
#pragma warning(disable : 5205)
#endif

namespace MRPlatExt
{
	void FSpatialAnchorPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FSpatialAnchorPlugin::Unregister()
	{
#if HL_ANCHOR_STORE_AVAILABLE 
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
	}

	bool FSpatialAnchorPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME);
		return true;
	}

	
	bool FSpatialAnchorPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) 
	{
#if HL_ANCHOR_STORE_AVAILABLE 
		OutExtensions.Add(XR_MSFT_PERCEPTION_ANCHOR_INTEROP_PREVIEW_EXTENSION_NAME);
#endif
		return true;
	}


	const void* FSpatialAnchorPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) 
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorMSFT", (PFN_xrVoidFunction*) &xrCreateSpatialAnchorMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorSpaceMSFT",(PFN_xrVoidFunction*) &xrCreateSpatialAnchorSpaceMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrDestroySpatialAnchorMSFT", (PFN_xrVoidFunction*) &xrDestroySpatialAnchorMSFT));

#if HL_ANCHOR_STORE_AVAILABLE 
		bIsLocalAnchorStoreSupported = IOpenXRHMDPlugin::Get().IsExtensionEnabled(XR_MSFT_PERCEPTION_ANCHOR_INTEROP_PREVIEW_EXTENSION_NAME);
		
		if (bIsLocalAnchorStoreSupported)
		{
			XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorFromPerceptionAnchorMSFT", (PFN_xrVoidFunction*) &xrCreateSpatialAnchorFromPerceptionAnchorMSFT));
			XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrTryGetPerceptionAnchorFromSpatialAnchorMSFT", (PFN_xrVoidFunction*) &xrTryGetPerceptionAnchorFromSpatialAnchorMSFT));

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
		AnchorMSFT->AnchorId = AnchorId;
		AnchorMSFT->Space = AnchorSpace;

		NewPin->SetNativeResource(reinterpret_cast<void*>(AnchorMSFT));

		return true;
	}

	void FSpatialAnchorPlugin::OnRemovePin(class UARPin* Pin) 
	{
		if (void* nativeResource = Pin->GetNativeResource())
		{
			SAnchorMSFT* AnchorMSFT = reinterpret_cast<SAnchorMSFT*>(nativeResource);
			xrDestroySpatialAnchorMSFT(AnchorMSFT->AnchorId);
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
		return bIsLocalAnchorStoreSupported;
	}

	bool FSpatialAnchorPlugin::ArePinsReadyToLoad() 
	{
#if HL_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		return m_spatialAnchorStore != nullptr;
#else
		return false;
#endif
	}

	void FSpatialAnchorPlugin::LoadARPins(XrSession InSession, TFunction<UARPin*(FName)> OnCreatePin) 
	{
#if HL_ANCHOR_STORE_AVAILABLE 
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
			AnchorMSFT->AnchorId = AnchorId;
			AnchorMSFT->Space = AnchorSpace;

			NewPin->SetNativeResource(reinterpret_cast<void*>(AnchorMSFT));
		}
#endif
	}

	bool FSpatialAnchorPlugin::SaveARPin(XrSession InSession, FName InName, UARPin* Pin) 
	{
#if HL_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return false; }

		void* nativeResource = Pin->GetNativeResource();
		if (nativeResource == nullptr) { return false; }

		SAnchorMSFT* AnchorMSFT = reinterpret_cast<SAnchorMSFT*>(nativeResource);
		XrResult result;

		SpatialAnchor wmrAnchor = nullptr;
		result = xrTryGetPerceptionAnchorFromSpatialAnchorMSFT(InSession, AnchorMSFT->AnchorId, reinterpret_cast<::IUnknown**>(winrt::put_abi(wmrAnchor)));
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
#if HL_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return; }

		const FString SaveId = InName.ToString().ToLower();
		m_spatialAnchorStore.Remove(*SaveId);
#endif
	}

	void FSpatialAnchorPlugin::RemoveAllSavedARPins(XrSession InSession)
	{
#if HL_ANCHOR_STORE_AVAILABLE 
		std::lock_guard<std::mutex> lock(m_spatialAnchorStoreLock);
		if (m_spatialAnchorStore == nullptr) { return; }

		m_spatialAnchorStore.Clear();
#endif
	}

}	 // namespace MRPlatExt

#if HL_ANCHOR_STORE_AVAILABLE 
#pragma warning(pop)
#endif

