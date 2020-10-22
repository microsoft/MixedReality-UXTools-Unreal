#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "OpenXRCommon.h"
#include "OpenXRCore.h"
#include "IOpenXRARModule.h"
#include "OpenXRARPlugin.h"

#include "HeadMountedDisplayTypes.h"
#include "ARTypes.h"
#include "ARSessionConfig.h"
#include "WindowsMixedRealityInteropUtility.h"

#include "IXRTrackingSystem.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <mutex>
#include <map>

#include <winrt/windows.foundation.Collections.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Perception.Spatial.h>
#include <winrt/Windows.Perception.Spatial.Surfaces.h>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace MRPlatExt
{
	class WMRAnchorLocalizationData : public TSharedFromThis<WMRAnchorLocalizationData>
	{
	public:
		WMRAnchorLocalizationData(XrSpace AnchorSpace, winrt::Windows::Perception::Spatial::SpatialCoordinateSystem CoordinateSystem)
			: AnchorSpace(AnchorSpace)
			, CoordinateSystem(CoordinateSystem)
		{
		}

		~WMRAnchorLocalizationData()
		{
			xrDestroySpace(AnchorSpace);
		}

		XrSpace AnchorSpace;
		winrt::Windows::Perception::Spatial::SpatialCoordinateSystem CoordinateSystem;
	};

	struct MeshLocalizationData
	{
		FTransform LastKnownTransform = FTransform::Identity;
		EARTrackingState LastKnownTrackingState = EARTrackingState::NotTracking;
		winrt::Windows::Perception::Spatial::SpatialCoordinateSystem CoordinateSystem;
	};

	class FSpatialMappingPlugin : public IOpenXRExtensionPlugin, public IOpenXRCustomCaptureSupport
	{
	public:
		void Register();
		void Unregister();

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		const void* OnBeginSession(XrSession InSession, const void* InNext) override;
		void PostSyncActions(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		bool OnToggleARCapture(const bool On) override;
		IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override;

	private:
		std::mutex MeshRefsLock;
		winrt::event_token OnChangeEventToken;

		bool bGenerateSRMeshes = false;
		float VolumeSize = 20.0f;
		float TriangleDensity = 500.0f;
		unsigned int MeshToLocalizeThisFrame = 0;

		const int WarnAfterThisManyMeshes = 100;

		PFN_xrCreateSpatialAnchorFromPerceptionAnchorMSFT xrCreateSpatialAnchorFromPerceptionAnchorMSFT;
		PFN_xrCreateSpatialAnchorSpaceMSFT xrCreateSpatialAnchorSpaceMSFT;
		PFN_xrDestroySpatialAnchorMSFT xrDestroySpatialAnchorMSFT;

		winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus> RequestAccessOperation = nullptr;

		// Map of all observed mesh ID's to their coordinate systems.  Used for localizing previously observed meshes.
		std::map<FGuid, MeshLocalizationData> UniqueMeshes;
		// Map of anchor data to use when localizing new spatial mapping meshes.
		std::map<FGuid, TSharedPtr<WMRAnchorLocalizationData>> AnchorLocalizationData;

		IXRTrackingSystem* XRTrackingSystem = nullptr;
		IOpenXRARTrackedMeshHolder* TrackedMeshHolder = nullptr;
		winrt::Windows::Perception::Spatial::Surfaces::SpatialSurfaceObserver SurfaceObserver = nullptr;
		winrt::Windows::Perception::Spatial::Surfaces::SpatialSurfaceMeshOptions MeshOptions = nullptr;

		void OnStartARSession(class UARSessionConfig* SessionConfig) override;

		bool StartMeshObserver();
		void StopMeshObserver();

		bool FindMeshTransform(XrSpace AnchorSpace, XrTime DisplayTime, XrSpace TrackingSpace, FTransform MeshToCachedAnchorTransform, FTransform& Transform);
		bool GetTransformBetweenCoordinateSystems(winrt::Windows::Perception::Spatial::SpatialCoordinateSystem From, winrt::Windows::Perception::Spatial::SpatialCoordinateSystem To, FTransform& Transform);
		bool LocateSpatialMeshInTrackingSpace(const FGuid& MeshID, winrt::Windows::Perception::Spatial::SpatialCoordinateSystem MeshCoordinateSystem, XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace, FTransform& Transform);

		void UpdateBoundingVolume();
		void CopyMeshData(FOpenXRMeshUpdate* MeshUpdate, winrt::Windows::Perception::Spatial::Surfaces::SpatialSurfaceMesh SurfaceMesh);
		void OnSurfacesChanged(winrt::Windows::Perception::Spatial::Surfaces::SpatialSurfaceObserver Observer, winrt::Windows::Foundation::IInspectable);
	};
}	 // namespace MRPlatExt

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
