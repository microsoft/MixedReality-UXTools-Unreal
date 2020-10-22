#include "QRTrackingPlugin.h"
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "OpenXRCore.h"
#include "OpenXRARPlugin.h"
#include "IXRTrackingSystem.h"

#include "WindowsMixedRealityInteropUtility.h"

#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Surfaces;
using namespace winrt::Microsoft::MixedReality::QR;

#pragma warning(push)
#pragma warning(disable : 5205)

namespace MRPlatExt
{
	void FQRTrackingPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), this);
#if PLATFORM_WINDOWS
		const FString DllName = "Microsoft.MixedReality.QR.dll";
		const FString LibrariesDir = FPaths::ProjectPluginsDir() / "MRPlatExt"/ THIRDPARTY_BINARY_SUBFOLDER;

		FPlatformProcess::PushDllDirectory(*LibrariesDir);
		if (!FPlatformProcess::GetDllHandle(*DllName))
		{
			UE_LOG(LogHMD, Warning, TEXT("Dll \'%s\' can't be loaded from \'%s\'"), *DllName, *LibrariesDir);
		}
		FPlatformProcess::PopDllDirectory(*LibrariesDir);
#endif
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
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialGraphNodeSpaceMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialGraphNodeSpaceMSFT));

		QRCodeHolder = IModularFeatures::Get().GetModularFeatureImplementations<IOpenXRARQRCodeHolder>(IOpenXRARQRCodeHolder::GetModularFeatureName())[0];

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
		auto QRCode = new FOpenXRQRCodeData;
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

		QRCodeHolder->QRCodeAdded(QRCode);
	}

	void FQRTrackingPlugin::OnUpdated(QRCodeWatcher sender, QRCodeUpdatedEventArgs args)
	{
		//OnUpdated works not good, so it's replaced by loop in PostSyncActions
	}

	void FQRTrackingPlugin::OnRemoved(QRCodeWatcher sender, QRCodeRemovedEventArgs args)
	{
		auto QRCode = new FOpenXRQRCodeData;
		auto InCode = args.Code();

		QRCode->Id = WMRUtility::GUIDToFGuid(InCode.Id());
		QRCode->Timestamp = FPlatformTime::Seconds();

		{
			FScopeLock Lock(&QRCodeContextsMutex);
			QRCodeContexts.Remove(QRCode->Id);
		}
		

		QRCodeHolder->QRCodeRemoved(QRCode);
	}

	void FQRTrackingPlugin::OnEnumerationCompleted(QRCodeWatcher sender, winrt::Windows::Foundation::IInspectable args)
	{

	}

	void FQRTrackingPlugin::PostSyncActions(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
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

			auto OutCode = new FOpenXRQRCodeData;

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

				check(sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId) == sizeof(FGuid));
				FMemory::Memcpy(&SpatialGraphNodeSpaceCreateInfo.nodeId, &Context->SpatialGraphNodeId, sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId));

				XR_ENSURE(xrCreateSpatialGraphNodeSpaceMSFT(InSession, &SpatialGraphNodeSpaceCreateInfo, &Context->Space));
			}

			XrSpaceLocation SpaceLocation{ XR_TYPE_SPACE_LOCATION };
			XR_ENSURE(xrLocateSpace(Context->Space, TrackingSpace, DisplayTime, &SpaceLocation));
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

			QRCodeHolder->QRCodeUpdated(OutCode);
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

}	 // namespace MRPlatExt

#pragma warning(pop)
#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
