// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "QRTrackingPlugin.h"
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "OpenXRCore.h"
#include "IOpenXRARTrackedGeometryHolder.h"
#include "IXRTrackingSystem.h"

#include "WindowsMixedRealityInteropUtility.h"

#include <winrt/Windows.Foundation.Collections.h>

#if WITH_EDITOR
#include "Editor.h"
#endif

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Surfaces;
using namespace winrt::Microsoft::MixedReality::QR;

namespace MicrosoftOpenXR
{
	void FQRTrackingPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), this);
#if PLATFORM_WINDOWS
		const FString PluginBaseDir = IPluginManager::Get().FindPlugin("MicrosoftOpenXR")->GetBaseDir();
		const FString DllName = "Microsoft.MixedReality.QR.dll";
		const FString LibrariesDir = PluginBaseDir / THIRDPARTY_BINARY_SUBFOLDER;

		FPlatformProcess::PushDllDirectory(*LibrariesDir);
		if (!FPlatformProcess::GetDllHandle(*DllName))
		{
			UE_LOG(LogHMD, Warning, TEXT("Dll \'%s\' can't be loaded from \'%s\'"), *DllName, *LibrariesDir);
		}
		FPlatformProcess::PopDllDirectory(*LibrariesDir);
#endif
	
#if WITH_EDITOR
		// When PIE stops (when remoting), ShutdownModule is not called.
		// This will prevent QR from working on subsequent plays.
		FEditorDelegates::EndPIE.AddRaw(this, &FQRTrackingPlugin::HandleEndPIE);
#endif
	}

	void FQRTrackingPlugin::HandleEndPIE(const bool InIsSimulating)
	{
		Unregister();
	}

	void FQRTrackingPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), this);
		StopQRCodeWatcher();
	}

	bool FQRTrackingPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SPATIAL_GRAPH_BRIDGE_EXTENSION_NAME);
		return true;
	}


	const void* FQRTrackingPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialGraphNodeSpaceMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialGraphNodeSpaceMSFT));

		QRCodeHolder = IModularFeatures::Get().GetModularFeatureImplementations<IOpenXRARTrackedGeometryHolder>(IOpenXRARTrackedGeometryHolder::GetModularFeatureName())[0];

		ensure(QRCodeHolder != nullptr);

		static FName SystemName(TEXT("OpenXR"));
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
		{
			XRTrackingSystem = GEngine->XRSystem.Get();
		}

		ensure(XRTrackingSystem != nullptr);

		return InNext;
	}

	bool FQRTrackingPlugin::OnToggleARCapture(const bool On)
	{
		if (On)
		{
			return StartQRCodeWatcher();
		}
		else
		{
			StopQRCodeWatcher();
		}
		return true;
	}

	bool FQRTrackingPlugin::IsEnabled() const
	{
		std::lock_guard<std::recursive_mutex> lock(QRCodeRefsLock);
		return QRTrackerInstance != nullptr;
	}

	bool FQRTrackingPlugin::StartQRCodeWatcher()
	{
		std::lock_guard<std::recursive_mutex> lock(QRCodeRefsLock);

		// Create the tracker and register the callbacks
		if (QRTrackerInstance == nullptr)
		{
			try
			{
				if (!QRCodeWatcher::IsSupported())
				{
					UE_LOG(LogHMD, Log, TEXT("QRCodeWatcher is not supported."));
					return false;
				}
			}
			catch (winrt::hresult_error e)
			{
				UE_LOG(LogHMD, Error, TEXT("QRCodeWatcher::IsSupported failed with error: %d"), e.code().value);
				return false;
			}

			m_QRTrackerAsyncOperation = QRCodeWatcher::RequestAccessAsync();
			m_QRTrackerAsyncOperation.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
			{
				if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed)
				{
					if (asyncInfo.GetResults() == QRCodeWatcherAccessStatus::Allowed)
					{
						std::lock_guard<std::recursive_mutex> lock(QRCodeRefsLock);

						QRTrackerInstance = QRCodeWatcher();
						OnAddedEventToken = QRTrackerInstance.Added(winrt::auto_revoke, [=](auto&& sender, auto&& args) { OnAdded(sender, args); });
						OnUpdatedEventToken = QRTrackerInstance.Updated(winrt::auto_revoke, [=](auto&& sender, auto&& args) { OnUpdated(sender, args); });
						OnRemovedEventToken = QRTrackerInstance.Removed(winrt::auto_revoke, [=](auto&& sender, auto&& args) { OnRemoved(sender, args); });
						OnEnumerationCompletedToken = QRTrackerInstance.EnumerationCompleted(winrt::auto_revoke, [=](auto&& sender, auto&& args) { OnEnumerationCompleted(sender, args); });

						// Start the tracker
						QRTrackerInstance.Start();

						m_QRTrackerAsyncOperation = nullptr;
					}
					else
					{
						UE_LOG(LogHMD, Log, TEXT("QRTracker access request returns error: %d"), asyncInfo.GetResults());
					}
				}
			});
		}
		return true;
	}

	void FQRTrackingPlugin::StopQRCodeWatcher()
	{
		std::lock_guard<std::recursive_mutex> lock(QRCodeRefsLock);
		if (m_QRTrackerAsyncOperation != nullptr && m_QRTrackerAsyncOperation.Status() != winrt::Windows::Foundation::AsyncStatus::Completed)
		{
			m_QRTrackerAsyncOperation.Cancel();
		}

		if (QRTrackerInstance != nullptr)
		{
			OnAddedEventToken.revoke();
			OnUpdatedEventToken.revoke();
			OnRemovedEventToken.revoke();
			OnEnumerationCompletedToken.revoke();

			// Stop the tracker
			QRTrackerInstance.Stop();
			QRTrackerInstance = nullptr;
		}

		{
			FScopeLock Lock(&QRCodeContextsMutex);
			QRCodeContexts.Empty();
		}
	}

	void FQRTrackingPlugin::OnAdded(QRCodeWatcher sender, QRCodeAddedEventArgs args)
	{
		auto QRCode = MakeShared<FOpenXRQRCodeData>();
		auto InCode = args.Code();

		QRCode->Id = WMRUtility::GUIDToFGuid(InCode.Id());
		QRCode->Version = (int32_t)InCode.Version();
		QRCode->QRCode = InCode.Data().c_str();
		QRCode->Size = FVector2D(InCode.PhysicalSideLength()) * XRTrackingSystem->GetWorldToMetersScale();
		QRCode->Timestamp = FPlatformTime::Seconds();
		QRCode->TrackingState = EARTrackingState::NotTracking;
		QRCode->LocalToTrackingTransform = FTransform::Identity; //OnAdded returns no actual pose

		auto Context = MakeShared<QRCodeContext, ESPMode::ThreadSafe>();
		Context->SpatialGraphNodeId = WMRUtility::GUIDToFGuid(InCode.SpatialGraphNodeId());

		{
			FScopeLock Lock(&QRCodeContextsMutex);
			QRCodeContexts.FindOrAdd(QRCode->Id) = Context;
		}

		QRCodeHolder->ARTrackedGeometryAdded(QRCode);
	}

	void FQRTrackingPlugin::OnUpdated(QRCodeWatcher sender, QRCodeUpdatedEventArgs args)
	{
		// Mark the tracked QRCode as updated so it will be located in UpdateDeviceLocations.
		std::lock_guard<std::recursive_mutex> lock(QRCodeRefsLock);
		if (QRTrackerInstance == nullptr) { return; }

		FScopeLock Lock(&QRCodeContextsMutex);
		FGuid Guid = WMRUtility::GUIDToFGuid(args.Code().Id());
		if (QRCodeContexts.Contains(Guid))
		{
			QRCodeContextPtr& Context = QRCodeContexts[Guid];
			Context->HasChanged = true;
		}
	}

	void FQRTrackingPlugin::OnRemoved(QRCodeWatcher sender, QRCodeRemovedEventArgs args)
	{
		auto QRCode = MakeShared<FOpenXRQRCodeData>();
		auto InCode = args.Code();

		QRCode->Id = WMRUtility::GUIDToFGuid(InCode.Id());
		QRCode->Timestamp = FPlatformTime::Seconds();

		{
			FScopeLock Lock(&QRCodeContextsMutex);
			QRCodeContexts.Remove(QRCode->Id);
		}
		

		QRCodeHolder->ARTrackedGeometryRemoved(QRCode);
	}

	void FQRTrackingPlugin::OnEnumerationCompleted(QRCodeWatcher sender, winrt::Windows::Foundation::IInspectable args)
	{

	}

	void FQRTrackingPlugin::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		std::lock_guard<std::recursive_mutex> lock(QRCodeRefsLock);
		if (QRTrackerInstance == nullptr) { return; }

		for(auto&& InCode : QRTrackerInstance.GetList())
		{
			FGuid OutGuid = WMRUtility::GUIDToFGuid(InCode.Id());
			QRCodeContextPtr Context;
			{
				FScopeLock Lock(&QRCodeContextsMutex);
				if (auto PtrPtr = QRCodeContexts.Find(OutGuid))
				{
					Context = *PtrPtr;
				}
				else
				{
					continue; //QRCode is being deleted
				}
			}

			if (!Context->HasChanged)
			{
				// Only update QR Codes that have updated.
				continue;
			}
			Context->HasChanged = false;

			auto OutCode = MakeShared<FOpenXRQRCodeData>();
			OutCode->Id = OutGuid;
			OutCode->Version = (int32_t)InCode.Version();
			OutCode->QRCode = InCode.Data().c_str();
			OutCode->Size = FVector2D(InCode.PhysicalSideLength()) * XRTrackingSystem->GetWorldToMetersScale();
			OutCode->Timestamp = FPlatformTime::Seconds();

			if (Context->Space == XR_NULL_HANDLE)
			{
				XrSpatialGraphNodeSpaceCreateInfoMSFT SpatialGraphNodeSpaceCreateInfo{ XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT };
				SpatialGraphNodeSpaceCreateInfo.nodeType = XR_SPATIAL_GRAPH_NODE_TYPE_STATIC_MSFT;
				SpatialGraphNodeSpaceCreateInfo.pose = ToXrPose(FTransform::Identity, XRTrackingSystem->GetWorldToMetersScale());

				check(sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId) == sizeof(winrt::guid));
				winrt::guid SourceGuid = WMRUtility::FGUIDToGuid(Context->SpatialGraphNodeId);
				FMemory::Memcpy(&SpatialGraphNodeSpaceCreateInfo.nodeId, &SourceGuid, sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId));

				XR_ENSURE_MSFT(xrCreateSpatialGraphNodeSpaceMSFT(InSession, &SpatialGraphNodeSpaceCreateInfo, &Context->Space));
			}

			XrSpaceLocation SpaceLocation{ XR_TYPE_SPACE_LOCATION };
			XR_ENSURE_MSFT(xrLocateSpace(Context->Space, TrackingSpace, DisplayTime, &SpaceLocation));
			const XrSpaceLocationFlags ValidFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
			if ((SpaceLocation.locationFlags & ValidFlags) == ValidFlags)
			{
				OutCode->LocalToTrackingTransform = ToFTransform(SpaceLocation.pose, XRTrackingSystem->GetWorldToMetersScale());
				OutCode->TrackingState = EARTrackingState::Tracking;
			}
			else
			{
				OutCode->TrackingState = EARTrackingState::NotTracking;
			}

			QRCodeHolder->ARTrackedGeometryUpdated(OutCode);
		}
	}

	IOpenXRCustomCaptureSupport* FQRTrackingPlugin::GetCustomCaptureSupport(const EARCaptureType CaptureType)
	{
		if (CaptureType == EARCaptureType::QRCode)
		{
			return this;
		}
		return nullptr;
	}


	FQRTrackingPlugin::QRCodeContext::~QRCodeContext()
	{
		if (Space != XR_NULL_HANDLE)
		{
			xrDestroySpace(Space);
		}
	}

}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
