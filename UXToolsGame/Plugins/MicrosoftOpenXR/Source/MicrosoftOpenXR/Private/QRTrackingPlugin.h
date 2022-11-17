// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "Interfaces/IPluginManager.h"

#include "OpenXRCommon.h"
#include "ARTypes.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Surfaces.h>
#include <winrt/Microsoft.MixedReality.QR.h>

#include <mutex>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

class IOpenXRARTrackedGeometryHolder;
struct FOpenXRQRCodeData;


namespace MicrosoftOpenXR
{
	class FQRTrackingPlugin : public IOpenXRExtensionPlugin, public IOpenXRCustomCaptureSupport
	{
	public:
		void Register();
		void Unregister();

		virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		virtual IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override;

		bool OnToggleARCapture(const bool bOnOff) override;
		bool IsEnabled() const;
	private:

		PFN_xrCreateSpatialGraphNodeSpaceMSFT xrCreateSpatialGraphNodeSpaceMSFT;

		bool StartQRCodeWatcher();
		void StopQRCodeWatcher();

		void HandleEndPIE(const bool InIsSimulating);

		// WinRT handlers
		void OnAdded(winrt::Microsoft::MixedReality::QR::QRCodeWatcher sender, winrt::Microsoft::MixedReality::QR::QRCodeAddedEventArgs args);
		void OnUpdated(winrt::Microsoft::MixedReality::QR::QRCodeWatcher sender, winrt::Microsoft::MixedReality::QR::QRCodeUpdatedEventArgs args);
		void OnRemoved(winrt::Microsoft::MixedReality::QR::QRCodeWatcher sender, winrt::Microsoft::MixedReality::QR::QRCodeRemovedEventArgs args);
		void OnEnumerationCompleted(winrt::Microsoft::MixedReality::QR::QRCodeWatcher sender, winrt::Windows::Foundation::IInspectable args);

		winrt::Microsoft::MixedReality::QR::QRCodeWatcher QRTrackerInstance = nullptr;
		winrt::Windows::Foundation::IAsyncOperation < winrt::Microsoft::MixedReality::QR::QRCodeWatcherAccessStatus > m_QRTrackerAsyncOperation = nullptr;

		winrt::Microsoft::MixedReality::QR::QRCodeWatcher::Added_revoker OnAddedEventToken;
		winrt::Microsoft::MixedReality::QR::QRCodeWatcher::Updated_revoker OnUpdatedEventToken;
		winrt::Microsoft::MixedReality::QR::QRCodeWatcher::Removed_revoker OnRemovedEventToken;
		winrt::Microsoft::MixedReality::QR::QRCodeWatcher::EnumerationCompleted_revoker OnEnumerationCompletedToken;

		mutable std::recursive_mutex QRCodeRefsLock;

		struct QRCodeContext
		{
			EARTrackingState TrackingState = EARTrackingState::Unknown;
			FGuid SpatialGraphNodeId;
			XrSpace Space = XR_NULL_HANDLE;
			bool HasChanged = false;

			~QRCodeContext();
		};

		typedef TSharedPtr<QRCodeContext, ESPMode::ThreadSafe> QRCodeContextPtr;

		IOpenXRARTrackedGeometryHolder* QRCodeHolder;

		class IXRTrackingSystem* XRTrackingSystem = nullptr;

		TMap<FGuid, QRCodeContextPtr > QRCodeContexts;
		FCriticalSection QRCodeContextsMutex;
	};
}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
