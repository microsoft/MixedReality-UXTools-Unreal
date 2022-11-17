// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ARBlueprintLibrary.h"
#include "Engine.h"
#include "IOpenXRARModule.h"
#include "IOpenXRARTrackedGeometryHolder.h"
#include "IXRTrackingSystem.h"
#include "MicrosoftOpenXR.h"
#include "OpenXRCore.h"
#include "SceneUnderstandingUtility.h"
#include "TrackedGeometryCollision.h"
#include "UniqueHandle.h"

#if SUPPORTS_REMOTING
#include "openxr_msft_holographic_remoting.h"
#endif

class IOpenXRARTrackedMeshHolder;

namespace MicrosoftOpenXR
{
	enum class EScanState
	{
		Idle,
		Waiting,
		Processing,
		AddMeshesToScene,
		Locating
	};

	struct FPlaneData
	{
		FGuid MeshGuid;
	};

	struct FPlaneUpdate
	{
		FGuid MeshGuid;
		EARObjectClassification Type;
		FVector3f Extent;
		TArray<FVector3f> Vertices;
		TArray<MRMESH_INDEX_TYPE> Indices;
	};

	struct FSceneUpdate
	{
		FSceneHandle Scene;
		TMap<XrUuidMSFT, FPlaneUpdate> Planes;
		TArray<XrUuidMSFT> PlaneUuids;
		TMap<FGuid, TrackedGeometryCollision> PlaneCollisionInfo;
		TMap<FGuid, TrackedGeometryCollision> MeshCollisionInfo;
	};

	class FSceneUnderstandingBase : public IOpenXRExtensionPlugin, public IOpenXRCustomCaptureSupport
	{
	public:
		void Register();
		void Unregister();

		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		const void* OnBeginSession(XrSession InSession, const void* InNext) override;

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		bool OnToggleARCapture(const bool bOnOff) override;

		IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override = 0;
		void OnStartARSession(class UARSessionConfig* SessionConfig) override;

		void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		TArray<FARTraceResult> OnLineTraceTrackedObjects(
			const TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe> ARCompositionComponent, const FVector Start,
			const FVector End, const EARLineTraceChannels TraceChannels) override;

		bool CanDetectPlanes();

	protected:
		virtual XrSceneComputeConsistencyMSFT GetSceneComputeConsistency() = 0;
		virtual TArray<XrSceneComputeFeatureMSFT> GetSceneComputeFeatures(class UARSessionConfig* SessionConfig) = 0;

	private:
		void UpdateObjectLocations(XrTime DisplayTime, XrSpace TrackingSpace);

		void ProcessSceneUpdate(FSceneUpdate&& SceneUpdate, XrTime DisplayTime, XrSpace TrackingSpace);

		void HandleEndPIE(const bool InIsSimulating);
		void Stop();

		void ComputeNewScene(XrTime DisplayTime);

		ExtensionDispatchTable Ext{};

		FSceneObserverHandle SceneObserver;
		FSceneHandle LocatingScene;
		FSpaceHandle ViewSpace;
		EScanState ScanState{ EScanState::Idle };

		TArray<XrSceneComputeFeatureMSFT> ComputeFeatures;
		TArray<XrScenePlaneAlignmentTypeMSFT> PlaneAlignmentFilters;

		// Members for reading scene components
		TArray<XrUuidMSFT> UuidsToLocate;
		TMap<XrUuidMSFT, FPlaneUpdate> Planes;
		TArray<XrSceneComponentLocationMSFT> Locations;
		TMap<XrUuidMSFT, FPlaneData> PreviousPlanes;
		int UuidToLocateThisFrame = 0;
		int UuidsToLocatePerFrame = 5;

		TMap<FGuid, TrackedGeometryCollision> PlaneCollisionInfo;
		TMap<FGuid, TrackedGeometryCollision> MeshCollisionInfo;

		TFuture<TSharedPtr<FSceneUpdate>> SceneUpdateFuture;

		class IXRTrackingSystem* XRTrackingSystem = nullptr;
		IOpenXRARTrackedMeshHolder* TrackedMeshHolder = nullptr;
		XrNewSceneComputeInfoMSFT SceneComputeInfo{ XR_TYPE_NEW_SCENE_COMPUTE_INFO_MSFT };
		XrSceneOrientedBoxBoundMSFT SceneBox;
		XrSceneSphereBoundMSFT SceneSphere;
		float SphereBoundRadius = 10.0f;	// meters
		float BoundHeight = 0.0f;			// meters
		bool bShouldStartSceneUnderstanding = false;
		bool bARSessionStarted = false;
		bool bCanDetectPlanes = false;
	};
}
