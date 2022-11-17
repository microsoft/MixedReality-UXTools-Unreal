// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "OpenXRCommon.h"

#if (PLATFORM_WINDOWS || PLATFORM_HOLOLENS)
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

#include <mutex>
#define WINRT_ANCHOR_STORE_AVAILABLE 1
#else
#define WINRT_ANCHOR_STORE_AVAILABLE 0
#endif

namespace MicrosoftOpenXR
{
	struct SAnchorMSFT
	{
		XrSpatialAnchorMSFT Anchor;
		XrSpace Space;
	};

	class FSpatialAnchorPlugin : public IOpenXRExtensionPlugin, public IOpenXRCustomAnchorSupport
	{
	public:
		void Register();
		void Unregister();

		virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		virtual bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		virtual const void* OnBeginSession(XrSession InSession, const void* InNext) override;

		virtual IOpenXRCustomAnchorSupport* GetCustomAnchorSupport() override;

		virtual bool OnPinComponent(class UARPin* Pin, XrSession InSession, XrSpace TrackingSpace, XrTime DisplayTime, float worldToMeterScale) override;

		virtual void OnRemovePin(class UARPin* Pin) override;

		virtual void OnUpdatePin(class UARPin* Pin, XrSession InSession, XrSpace TrackingSpace, XrTime DisplayTime, float worldToMeterScale) override;

		virtual bool IsLocalPinSaveSupported() const override;

		bool IsAnchorStoreReady();
		virtual bool ArePinsReadyToLoad() override;

		virtual void LoadARPins(XrSession InSession, TFunction<UARPin*(FName)> OnCreatePin) override;

		virtual bool SaveARPin(XrSession InSession, FName InName, UARPin* InPin) override;

		virtual void RemoveSavedARPin(XrSession InSession, FName InName) override;

		virtual void RemoveAllSavedARPins(XrSession InSession) override;

		bool GetPerceptionAnchorFromOpenXRAnchor(XrSpatialAnchorMSFT AnchorId, ::IUnknown** OutPerceptionAnchor);
		bool StorePerceptionAnchor(const FString& InPinId, ::IUnknown* InPerceptionAnchor);

	private:
		XrSession Session;

		PFN_xrCreateSpatialAnchorMSFT xrCreateSpatialAnchorMSFT;
		PFN_xrDestroySpatialAnchorMSFT xrDestroySpatialAnchorMSFT;
		PFN_xrCreateSpatialAnchorSpaceMSFT xrCreateSpatialAnchorSpaceMSFT;

		bool bIsAnchorPersistenceExtensionSupported = false;
		bool bIsPerceptionAnchorInteropExtensionSupported = false;

#if WINRT_ANCHOR_STORE_AVAILABLE
		PFN_xrCreateSpatialAnchorFromPerceptionAnchorMSFT xrCreateSpatialAnchorFromPerceptionAnchorMSFT;
		PFN_xrTryGetPerceptionAnchorFromSpatialAnchorMSFT xrTryGetPerceptionAnchorFromSpatialAnchorMSFT;

		std::mutex	m_spatialAnchorStoreLock;
		winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Perception::Spatial::SpatialAnchorStore>	m_spatialAnchorStoreAsyncOperation;
		winrt::Windows::Perception::Spatial::SpatialAnchorStore	m_spatialAnchorStore{ nullptr };
#endif

		PFN_xrCreateSpatialAnchorStoreConnectionMSFT xrCreateSpatialAnchorStoreConnectionMSFT;
		PFN_xrDestroySpatialAnchorStoreConnectionMSFT xrDestroySpatialAnchorStoreConnectionMSFT;
		PFN_xrPersistSpatialAnchorMSFT xrPersistSpatialAnchorMSFT;
		PFN_xrEnumeratePersistedSpatialAnchorNamesMSFT xrEnumeratePersistedSpatialAnchorNamesMSFT;
		PFN_xrCreateSpatialAnchorFromPersistedNameMSFT xrCreateSpatialAnchorFromPersistedNameMSFT;
		PFN_xrUnpersistSpatialAnchorMSFT xrUnpersistSpatialAnchorMSFT;
		PFN_xrClearSpatialAnchorStoreMSFT xrClearSpatialAnchorStoreMSFT;

		XrSpatialAnchorStoreConnectionMSFT SpatialAnchorStoreMSFT = XR_NULL_HANDLE;

		void OnEndPlay();
	};
}	 // namespace MicrosoftOpenXR
