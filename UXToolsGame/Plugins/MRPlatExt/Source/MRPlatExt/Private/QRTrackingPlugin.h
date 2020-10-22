#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
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

class IOpenXRARQRCodeHolder;
struct FOpenXRQRCodeData;


namespace MRPlatExt
{
	class FQRTrackingPlugin : public IOpenXRExtensionPlugin, public IOpenXRCustomCaptureSupport
	{
	public:
		void Register();
		void Unregister();

		virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		virtual void PostSyncActions(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		virtual IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override;

		bool OnToggleARCapture(const bool bOnOff);
		bool IsEnabled() const;
	private:

		PFN_xrCreateSpatialGraphNodeSpaceMSFT xrCreateSpatialGraphNodeSpaceMSFT;

		bool StartQRCodeWatcher();
		void StopQRCodeWatcher();

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

			~QRCodeContext();
		};

		typedef TSharedPtr<QRCodeContext, ESPMode::ThreadSafe> QRCodeContextPtr;

		IOpenXRARQRCodeHolder* QRCodeHolder;

		class IXRTrackingSystem* XRTrackingSystem = nullptr;

		TMap<FGuid, QRCodeContextPtr > QRCodeContexts;
		FCriticalSection QRCodeContextsMutex;
	};
}	 // namespace MRPlatExt

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
