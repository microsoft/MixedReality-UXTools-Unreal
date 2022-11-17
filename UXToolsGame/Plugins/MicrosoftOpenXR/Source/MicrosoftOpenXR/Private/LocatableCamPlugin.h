// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "OpenXRCommon.h"
#include "ARTypes.h"
#include "MicrosoftOpenXR.h"
#include "ARTextures.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <string>
#include <sstream>
#include <mutex>
#include <memory>

#include <unknwn.h>
#include <winrt/Windows.Media.Capture.h>
#include <winrt/Windows.Media.Capture.Frames.h>
#include <winrt/Windows.Perception.Spatial.h>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

class UOpenXRCameraImageTexture;

namespace MicrosoftOpenXR
{
	class FLocatableCamPlugin : public IOpenXRExtensionPlugin, public IOpenXRCustomCaptureSupport
	{
	public:

		FLocatableCamPlugin();
		~FLocatableCamPlugin();

		void Register();
		void Unregister();

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		virtual void OnStartARSession(class UARSessionConfig* SessionConfig) override;
		virtual void OnStopARSession() override;

		virtual IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override;


		FTransform GetCameraTransform() const override;

		bool GetPVCameraIntrinsics(FVector2D& focalLength, int& width, int& height, FVector2D& principalPoint, FVector& radialDistortion, FVector2D& tangentialDistortion) const;

		FVector GetWorldSpaceRayFromCameraPoint(FVector2D pixelCoordinate) const override;

		virtual bool OnGetCameraIntrinsics(FARCameraIntrinsics& OutCameraIntrinsics) const override;
		virtual class UARTexture* OnGetARTexture(EARTextureType TextureType) const override;
		virtual bool OnToggleARCapture(const bool bOnOff) override;
		bool IsEnabled() const override;

	private:

		class FSharedTextureHolder : public FGCObject
		{
		public:
			virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

			virtual FString GetReferencerName() const override
			{
				return TEXT("FSharedTextureHolder FGCObject");
			}

			UOpenXRCameraImageTexture* CameraImage = nullptr;
		};

		PFN_xrCreateSpatialGraphNodeSpaceMSFT xrCreateSpatialGraphNodeSpaceMSFT;

		void StartCameraCapture(int DesiredWidth, int DesiredHeight, int DesiredFPS);
		void StopCameraCapture();

		void OnFrameArrived(winrt::Windows::Media::Capture::Frames::MediaFrameReader SendingFrameReader, winrt::Windows::Media::Capture::Frames::MediaFrameArrivedEventArgs FrameArrivedArgs);


		/** Controls access to our references */
		mutable std::recursive_mutex RefsLock;
		/** The objects we need in order to receive frames of camera data */
		winrt::agile_ref<winrt::Windows::Media::Capture::MediaCapture> CameraCapture = nullptr;
		winrt::Windows::Media::Capture::Frames::MediaFrameReader CameraFrameReader = nullptr;
		winrt::Windows::Media::Capture::Frames::MediaFrameSource CameraFrameSource = nullptr;
		winrt::Windows::Media::Devices::Core::CameraIntrinsics CameraIntrinsics = nullptr;

		TOptional<TPair<FGuid, FTransform>> DynamicNode;

		winrt::Windows::Media::Capture::Frames::MediaFrameReader::FrameArrived_revoker OnFrameArrivedEvent;

		winrt::Windows::Foundation::IAsyncInfo AsyncInfo;

		std::shared_ptr<winrt::handle> SharedDXTexture;
		XrSpace Space = XR_NULL_HANDLE;

		class IXRTrackingSystem* XRTrackingSystem = nullptr;
		FARVideoFormat Format;

		FTransform PVCameraToWorldMatrix;
		TSharedPtr< FSharedTextureHolder> SharedTextureHolder;

		bool IsCameraCaptureDesired = false;
	};
}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
