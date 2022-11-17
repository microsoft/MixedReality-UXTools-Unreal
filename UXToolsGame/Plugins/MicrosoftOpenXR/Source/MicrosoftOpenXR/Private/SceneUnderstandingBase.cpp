// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "SceneUnderstandingBase.h"

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

#if WITH_EDITOR
#include "Editor.h"
#endif

class IOpenXRARTrackedMeshHolder;

namespace MicrosoftOpenXR
{
	inline FTransform GetPlaneTransform(const XrPosef& Pose, float WorldToMetersScale)
	{
		FTransform transform = ToFTransform(Pose, WorldToMetersScale);
		transform.ConcatenateRotation(FQuat(FVector(0, 1, 0), -HALF_PI));
		return transform;
	}

	TrackedGeometryCollision CreatePlaneGeometryCollision(const FVector3f& Extent)
	{
		TArray<FVector3f> Vertices;
		Vertices.Reset(4);
		Vertices.Add(Extent);
		Vertices.Add(FVector3f(Extent.X, -Extent.Y, Extent.Z));
		Vertices.Add(FVector3f(-Extent.X, -Extent.Y, Extent.Z));
		Vertices.Add(FVector3f(-Extent.X, Extent.Y, Extent.Z));

		// Two triangles
		TArray<MRMESH_INDEX_TYPE> Indices{ 0, 2, 1, 2, 0, 3 };
		return TrackedGeometryCollision(MoveTemp(Vertices), MoveTemp(Indices));
	}

	// This function should be called in a background thread.
	TSharedPtr<FSceneUpdate> LoadPlanes(const ExtensionDispatchTable& Ext, FSceneHandle Scene,
		TMap<XrUuidMSFT, FPlaneData>&& PlaneIdToMeshGuid, const TArray<XrScenePlaneAlignmentTypeMSFT>& PlaneAlignmentFilters,
		float WorldToMetersScale, XrSceneComponentTypeMSFT SceneComponentType)
	{
		// Get a map of SceneObject UUID to ObjectType.
		// Planes will determine their object classification by looking for their parent's UUID in this map.
		const TMap<XrUuidMSFT, XrSceneObjectTypeMSFT> ObjectTypeMap = GetObjectTypeMap(Scene.Handle(), Ext);

		auto SceneUpdate = MakeShared<FSceneUpdate>();
		auto& PlaneUpdates = SceneUpdate->Planes;
		auto& PlaneCollisionInfo = SceneUpdate->PlaneCollisionInfo;
		auto& MeshCollisionInfo = SceneUpdate->MeshCollisionInfo;

		TArray<XrSceneComponentMSFT> SceneComponents;
		TArray<XrScenePlaneMSFT> ScenePlanes;
		TArray<XrSceneMeshMSFT> SceneMeshes;
		
		bool FindingVisibleMeshes = SceneComponentType == XR_SCENE_COMPONENT_TYPE_VISUAL_MESH_MSFT;

		if (FindingVisibleMeshes)
		{
			GetSceneVisibleMeshes(Scene.Handle(), Ext, PlaneAlignmentFilters, SceneComponents, SceneMeshes);
			check(SceneComponents.Num() == SceneMeshes.Num());
		}
		else
		{
			GetScenePlanes(Scene.Handle(), Ext, PlaneAlignmentFilters, SceneComponents, ScenePlanes);
			check(SceneComponents.Num() == ScenePlanes.Num());
		}

		const int32_t Count = SceneComponents.Num();
		for (int32_t Index = 0; Index < Count; ++Index)
		{
			uint64_t MeshBufferID = 0;
			FVector2D PlaneExtents;

			// In the SU extension, the plane's mesh is part of the plane scene component so there is one UUID.
			// In Unreal the Plane and the mesh have separate GUIDs.
			// Therefore the plane will use the UUID as its GUID and the mesh will need to generate a GUID.
			const XrSceneComponentMSFT& SceneComponent = SceneComponents[Index];
			EARObjectClassification ObjectClassification;
			if (FindingVisibleMeshes)
			{
				const XrSceneMeshMSFT& SceneMesh = SceneMeshes[Index];
				MeshBufferID = SceneMesh.meshBufferId;
				// Visible meshes represent the SR mesh, categorize them as World.
				ObjectClassification = EARObjectClassification::World;
			}
			else
			{
				const XrScenePlaneMSFT& ScenePlane = ScenePlanes[Index];
				MeshBufferID = ScenePlane.meshBufferId;
				PlaneExtents = FVector2D(-ScenePlane.size.height, ScenePlane.size.width);
				ObjectClassification = GetObjectClassification(GetObjectType(ObjectTypeMap, SceneComponent.parentId));
			}

			const XrUuidMSFT& PlaneUuid = SceneComponent.id;
			const FGuid PlaneGuid = XrUuidMSFTToFGuid(PlaneUuid);

			FGuid MeshGuid{};
			// If meshBufferId is zero then the plane doesn't have a mesh. Likely because it wasn't requested.
			if (MeshBufferID != 0)
			{
				const auto* PrevPlaneData = PlaneIdToMeshGuid.Find(PlaneUuid);
				if (PrevPlaneData != nullptr && PrevPlaneData->MeshGuid.IsValid())
				{	 // Updated plane
					MeshGuid = PrevPlaneData->MeshGuid;
				}
				else
				{	 // New plane so generate a new GUID for the mesh
					MeshGuid = FGuid::NewGuid();
				}
			}
			FPlaneUpdate& PlaneUpdate = PlaneUpdates.Add(PlaneUuid);
			PlaneUpdate.MeshGuid = MeshGuid;
			PlaneUpdate.Type = ObjectClassification;

			if (!FindingVisibleMeshes)
			{
				// Visual mesh does not include planes
				PlaneUpdate.Extent = FVector3f(PlaneExtents.X, PlaneExtents.Y, 0) * WorldToMetersScale * 0.5f;
				PlaneCollisionInfo.Add(PlaneGuid, CreatePlaneGeometryCollision(PlaneUpdate.Extent));
			}

			if (MeshBufferID != 0)
			{
				TArray<XrVector3f> MeshVertices;
				ReadMeshBuffers(Scene.Handle(), Ext, MeshBufferID, MeshVertices, PlaneUpdate.Indices);
				PlaneUpdate.Vertices.SetNum(MeshVertices.Num());
				for (int32 i = 0; i < MeshVertices.Num(); i++)
				{
					const XrVector3f& SrcVertex = MeshVertices[i];
					PlaneUpdate.Vertices[i] = 
						FVector3f(-SrcVertex.z * WorldToMetersScale, SrcVertex.x * WorldToMetersScale, SrcVertex.y * WorldToMetersScale);
				}

				MeshCollisionInfo.Add(MeshGuid, TrackedGeometryCollision(PlaneUpdate.Vertices, PlaneUpdate.Indices));
			}
			// The planes and meshes will need to be located on the main thread using the DisplayTime.
		}
		SceneUpdate->Scene = MoveTemp(Scene);
		PlaneUpdates.GetKeys(SceneUpdate->PlaneUuids);
		return SceneUpdate;
	}

	void FSceneUnderstandingBase::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

#if WITH_EDITOR
		// When PIE stops (when remoting), ShutdownModule is not called.
		// This will leave a dangling scene handle that will crash the next play session.
		FEditorDelegates::EndPIE.AddRaw(this, &FSceneUnderstandingBase::HandleEndPIE);
#endif
	}

	void FSceneUnderstandingBase::HandleEndPIE(const bool InIsSimulating)
	{
		Unregister();
	}

	void FSceneUnderstandingBase::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
		
		Stop();
	}

	bool FSceneUnderstandingBase::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SCENE_UNDERSTANDING_EXTENSION_NAME);
		return true;
	}

	bool FSceneUnderstandingBase::OnToggleARCapture(const bool bOnOff)
	{
		if (bOnOff)
		{
			bShouldStartSceneUnderstanding = true;
		}
		else
		{
			Stop();
		}

		return true;
	}

	void FSceneUnderstandingBase::UpdateObjectLocations(XrTime DisplayTime, XrSpace TrackingSpace)
	{
		if (ScanState != EScanState::Locating)
		{
			return;
		}

		if (UuidsToLocate.Num() == 0 || LocatingScene.Handle() == XR_NULL_HANDLE)
		{
			ScanState = EScanState::Idle;
			return;
		}

		const float WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale();

		XrUuidMSFT Uuid = UuidsToLocate[UuidToLocateThisFrame];
		XrSceneComponentLocationMSFT Location = Locations[UuidToLocateThisFrame];

		TrackedMeshHolder->StartMeshUpdates();

		for (int i = 0; i < UuidsToLocatePerFrame; i++)
		{
			const FGuid PlaneGuid = XrUuidMSFTToFGuid(Uuid);
			FPlaneUpdate& Plane = Planes.FindChecked(Uuid);

			auto PlaneUpdate = MakeShared<FOpenXRMeshUpdate>();
			const FGuid Guid = XrUuidMSFTToFGuid(Uuid);
			PlaneUpdate->Id = XrUuidMSFTToFGuid(Uuid);
			PlaneUpdate->SpatialMeshUsageFlags = (EARSpatialMeshUsageFlags)((int32)EARSpatialMeshUsageFlags::Visible);
			if (IsPoseValid(Location.flags))
			{
				PlaneUpdate->TrackingState = EARTrackingState::Tracking;
				PlaneUpdate->LocalToTrackingTransform = GetPlaneTransform(Location.pose, WorldToMetersScale);
			}
			else
			{
				PlaneUpdate->TrackingState = EARTrackingState::NotTracking;
				// EARTrackingState::NotTracking should prevent the mesh from rendering. 
				// However, when ObjectUpdated is called: UARTrackedGeometry::UpdateTrackedGeometry assumes the mesh is being tracked.
				// This can cause a loss of tracking to place every mesh at the origin.
				// Workaround this by scaling the mesh to zero - when it is located again the transform will be corrected.
				PlaneUpdate->LocalToTrackingTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
			}
			TrackedMeshHolder->ObjectUpdated(MoveTemp(PlaneUpdate));

			if (const FPlaneData* PlaneData = PreviousPlanes.Find(Uuid); PlaneData != nullptr)
			{
				auto MeshUpdate = MakeShared<FOpenXRMeshUpdate>();
				const FGuid& MeshGuid = PlaneData->MeshGuid;

				MeshUpdate->Id = MeshGuid;
				MeshUpdate->SpatialMeshUsageFlags =
					(EARSpatialMeshUsageFlags)((int32)EARSpatialMeshUsageFlags::Visible |
						(int32)EARSpatialMeshUsageFlags::Collision);
				if (IsPoseValid(Location.flags))
				{
					MeshUpdate->TrackingState = EARTrackingState::Tracking;
					MeshUpdate->LocalToTrackingTransform = ToFTransform(Location.pose, WorldToMetersScale);
				}
				else
				{
					MeshUpdate->TrackingState = EARTrackingState::NotTracking;
					// EARTrackingState::NotTracking should prevent the mesh from rendering. 
					// However, when ObjectUpdated is called: UARTrackedGeometry::UpdateTrackedGeometry assumes the mesh is being tracked.
					// This can cause a loss of tracking to place every mesh at the origin.
					// Workaround this by scaling the mesh to zero - when it is located again the transform will be corrected.
					MeshUpdate->LocalToTrackingTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
				}

				TrackedMeshHolder->ObjectUpdated(MoveTemp(MeshUpdate));
			}

			UuidToLocateThisFrame++;
			if (UuidToLocateThisFrame >= UuidsToLocate.Num())
			{
				UuidToLocateThisFrame = 0;
				ScanState = EScanState::Idle;
				break;
			}
		}

		TrackedMeshHolder->EndMeshUpdates();
	}

	void FSceneUnderstandingBase::ProcessSceneUpdate(FSceneUpdate&& SceneUpdate, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		PlaneCollisionInfo = MoveTemp(SceneUpdate.PlaneCollisionInfo);
		MeshCollisionInfo = MoveTemp(SceneUpdate.MeshCollisionInfo);
		const float WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale();
		LocateObjects(SceneUpdate.Scene.Handle(), Ext, TrackingSpace, DisplayTime, SceneUpdate.PlaneUuids, Locations);

		// Remove any meshes that are no longer in the scene.
		TrackedMeshHolder->StartMeshUpdates();
		for (const auto& Elem : PreviousPlanes)
		{
			const XrUuidMSFT& PlaneUuid = Elem.Key;
			if (!SceneUpdate.Planes.Contains(PlaneUuid))
			{
				const FGuid& MeshGuid = Elem.Value.MeshGuid;
				if (MeshGuid.IsValid())
				{
					TrackedMeshHolder->RemoveMesh(MeshGuid);
				}
				TrackedMeshHolder->RemovePlane(XrUuidMSFTToFGuid(PlaneUuid));
			}
		}
		TrackedMeshHolder->EndMeshUpdates();

		PreviousPlanes.Reset();
		for (const auto& Elem : SceneUpdate.Planes)
		{
			PreviousPlanes.Add(Elem.Key, { Elem.Value.MeshGuid });
		}

		// Destroying a Scene is unexpectedly slow so destroy it on a background thread.
		AsyncTask(ENamedThreads::AnyThread, [Scene = std::move(LocatingScene), Ext = Ext]() mutable { Scene.Reset(); });

		LocatingScene = MoveTemp(SceneUpdate.Scene);
		UuidsToLocate = MoveTemp(SceneUpdate.PlaneUuids);
		Planes = MoveTemp(SceneUpdate.Planes);
	}

	void FSceneUnderstandingBase::OnStartARSession(class UARSessionConfig* SessionConfig)
	{
		float VolumeSize;
		if (GConfig->GetFloat(TEXT("/Script/HoloLensPlatformEditor.HoloLensTargetSettings"), TEXT("SpatialMeshingVolumeSize"),
			VolumeSize, GEngineIni))
		{
			SphereBoundRadius = VolumeSize / 2.0f;
		}

		float VolumeHeight;
		if (GConfig->GetFloat(TEXT("/Script/HoloLensSettings.SceneUnderstanding"), TEXT("SceneUnderstandingVolumeHeight"),
			VolumeHeight, GGameIni))
		{
			BoundHeight = VolumeHeight / 2.0f;
		}

		PlaneAlignmentFilters.Reset();
		if (SessionConfig->ShouldDoHorizontalPlaneDetection() && !SessionConfig->ShouldDoVerticalPlaneDetection())
		{
			PlaneAlignmentFilters.AddUnique(XR_SCENE_PLANE_ALIGNMENT_TYPE_HORIZONTAL_MSFT);
		}
		else if (SessionConfig->ShouldDoVerticalPlaneDetection() && !SessionConfig->ShouldDoHorizontalPlaneDetection())
		{
			PlaneAlignmentFilters.AddUnique(XR_SCENE_PLANE_ALIGNMENT_TYPE_VERTICAL_MSFT);
		}

		ComputeFeatures = GetSceneComputeFeatures(SessionConfig);

		bARSessionStarted = true;
	}

	TArray<FARTraceResult> FSceneUnderstandingBase::OnLineTraceTrackedObjects(
		const TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe> ARCompositionComponent, const FVector Start,
		const FVector End, const EARLineTraceChannels TraceChannels)
	{
		// Always hittest meshes, but only hittest planes if PlaneUsingExtent is enabled.
		// This is because some planes may be floating in space, like wall planes through an open door.
		bool HitTestPlanes = ((int32)TraceChannels & (int32)EARLineTraceChannels::PlaneUsingExtent) != 0;

		TArray<FARTraceResult> Results;
		TArray<UARMeshGeometry*> Meshes = UARBlueprintLibrary::GetAllGeometriesByClass<UARMeshGeometry>();
		for (UARMeshGeometry* Mesh : Meshes)
		{
			auto CollisionInfo = MeshCollisionInfo.Find(Mesh->UniqueId);
			if (CollisionInfo != nullptr)
			{
				FVector HitPoint, HitNormal;
				float HitDistance;
				if (CollisionInfo->Collides(Start, End, Mesh->GetLocalToWorldTransform(), HitPoint, HitNormal, HitDistance))
				{
					// Append a hit.  The calling function will then sort by HitDistance.
					Results.Add(FARTraceResult(ARCompositionComponent, HitDistance, TraceChannels,
						FTransform(HitNormal.ToOrientationQuat(), HitPoint), Mesh));
				}
			}
		}

		if (HitTestPlanes)
		{
			TArray<UARPlaneGeometry*> TrackedPlanes = UARBlueprintLibrary::GetAllGeometriesByClass<UARPlaneGeometry>();
			for (UARPlaneGeometry* Plane : TrackedPlanes)
			{
				auto CollisionInfo = PlaneCollisionInfo.Find(Plane->UniqueId);
				if (CollisionInfo != nullptr)
				{
					FVector HitPoint, HitNormal;
					float HitDistance;
					if (CollisionInfo->Collides(Start, End, Plane->GetLocalToWorldTransform(), HitPoint, HitNormal, HitDistance))
					{
						// Append a hit.  The calling function will then sort by HitDistance.
						Results.Add(FARTraceResult(ARCompositionComponent, HitDistance, TraceChannels,
							FTransform(HitNormal.ToOrientationQuat(), HitPoint), Plane));
					}
				}
			}
		}
		return Results;
	};

	const void* FSceneUnderstandingBase::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		XR_ENSURE(xrGetInstanceProcAddr(
			InInstance, "xrEnumerateSceneComputeFeaturesMSFT", (PFN_xrVoidFunction*)&Ext.xrEnumerateSceneComputeFeaturesMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(
			InInstance, "xrCreateSceneObserverMSFT", (PFN_xrVoidFunction*)&Ext.xrCreateSceneObserverMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(
			InInstance, "xrDestroySceneObserverMSFT", (PFN_xrVoidFunction*)&Ext.xrDestroySceneObserverMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSceneMSFT", (PFN_xrVoidFunction*)&Ext.xrCreateSceneMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrDestroySceneMSFT", (PFN_xrVoidFunction*)&Ext.xrDestroySceneMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrComputeNewSceneMSFT", (PFN_xrVoidFunction*)&Ext.xrComputeNewSceneMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(
			InInstance, "xrGetSceneComputeStateMSFT", (PFN_xrVoidFunction*)&Ext.xrGetSceneComputeStateMSFT));
		XR_ENSURE(
			xrGetInstanceProcAddr(InInstance, "xrGetSceneComponentsMSFT", (PFN_xrVoidFunction*)&Ext.xrGetSceneComponentsMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(
			InInstance, "xrLocateSceneComponentsMSFT", (PFN_xrVoidFunction*)&Ext.xrLocateSceneComponentsMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(
			InInstance, "xrGetSceneMeshBuffersMSFT", (PFN_xrVoidFunction*)&Ext.xrGetSceneMeshBuffersMSFT));

		// Check if Scene Understanding supports plane finding.
		if (UMicrosoftOpenXRFunctionLibrary::IsRemoting())
		{
			// When remoting, xrEnumerateSceneComputeFeaturesMSFT will only contain XR_SCENE_COMPUTE_FEATURE_PLANE_MSFT
			// If the app remoting player is running when the editor starts.
			// Otherwise, this will always be false even though the 2.8+ remoting runtime does support planes.
			// Workaround this by forcing this flag to true when remoting.
			bCanDetectPlanes = true;
		}
		else
		{
			uint32 FeatureCount;
			TArray<XrSceneComputeFeatureMSFT> SceneComputeFeatures;
			XR_ENSURE(Ext.xrEnumerateSceneComputeFeaturesMSFT(InInstance, InSystem, 0, &FeatureCount, nullptr));
			SceneComputeFeatures.AddUninitialized(FeatureCount);
			XR_ENSURE(Ext.xrEnumerateSceneComputeFeaturesMSFT(InInstance, InSystem, FeatureCount, &FeatureCount, SceneComputeFeatures.GetData()));

			bCanDetectPlanes = SceneComputeFeatures.Contains(XR_SCENE_COMPUTE_FEATURE_PLANE_MSFT);
		}

		return InNext;
	}

	bool FSceneUnderstandingBase::CanDetectPlanes()
	{
		return bCanDetectPlanes;
	}

	const void* FSceneUnderstandingBase::OnBeginSession(XrSession InSession, const void* InNext)
	{
		static FName SystemName(TEXT("OpenXR"));
		if (!GEngine->XRSystem.IsValid() || (GEngine->XRSystem->GetSystemName() != SystemName))
		{
			return InNext;
		}
		XRTrackingSystem = GEngine->XRSystem.Get();

		if (IOpenXRARModule::IsAvailable())
		{
			TrackedMeshHolder = IOpenXRARModule::Get().GetTrackedMeshHolder();
		}
		ViewSpace = CreateViewSpace(InSession);
		return InNext;
	}

	void FSceneUnderstandingBase::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		if (bShouldStartSceneUnderstanding && TrackedMeshHolder != nullptr && SceneObserver.Handle() == XR_NULL_HANDLE)
		{
			SceneObserver = CreateSceneObserver(Ext, InSession);
		}
		if (SceneObserver.Handle() == XR_NULL_HANDLE || TrackedMeshHolder == nullptr || XRTrackingSystem == nullptr || !bARSessionStarted)
		{
			return;
		}

		if (ScanState == EScanState::Idle)
		{
			if (bShouldStartSceneUnderstanding)
			{
				ComputeNewScene(DisplayTime);
				ScanState = EScanState::Waiting;
			}
			else
			{
				// Scene Understanding has been stopped, only locate any existing meshes.
				if (UuidsToLocate.Num() != 0 && LocatingScene.Handle() != XR_NULL_HANDLE)
				{
					LocateObjects(LocatingScene.Handle(), Ext, TrackingSpace, DisplayTime, UuidsToLocate, Locations);
					ScanState = EScanState::Locating;
				}
			}
		}
		else if (ScanState == EScanState::Waiting)
		{
			XrSceneComputeStateMSFT SceneComputeState;
			XR_ENSURE(Ext.xrGetSceneComputeStateMSFT(SceneObserver.Handle(), &SceneComputeState));
			if (SceneComputeState == XR_SCENE_COMPUTE_STATE_COMPLETED_WITH_ERROR_MSFT ||
				SceneComputeState == XR_SCENE_COMPUTE_STATE_NONE_MSFT)
			{
				ScanState = EScanState::Idle;
			}
			else if (SceneComputeState == XR_SCENE_COMPUTE_STATE_COMPLETED_MSFT)
			{
				XrSceneComponentTypeMSFT SceneComponentType = 
					GetSceneComputeConsistency() == XR_SCENE_COMPUTE_CONSISTENCY_OCCLUSION_OPTIMIZED_MSFT ?
					XR_SCENE_COMPONENT_TYPE_VISUAL_MESH_MSFT : XR_SCENE_COMPONENT_TYPE_OBJECT_MSFT;

				FSceneHandle Scene = CreateScene(Ext, SceneObserver.Handle());
				TPromise<TSharedPtr<FSceneUpdate>> Promise;
				SceneUpdateFuture = Promise.GetFuture();
				AsyncTask(ENamedThreads::AnyThread,
					[Ext = Ext, WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale(),
					PlaneAlignmentFilters = PlaneAlignmentFilters, Scene = MoveTemp(Scene),
					PlaneIdToMeshGuid = PreviousPlanes, Promise = MoveTemp(Promise),
					SceneComponentType = SceneComponentType]() mutable {
					Promise.SetValue(LoadPlanes(
						Ext, MoveTemp(Scene), MoveTemp(PlaneIdToMeshGuid), PlaneAlignmentFilters, 
						WorldToMetersScale, SceneComponentType));
				});
				ScanState = EScanState::Processing;
			}
		}
		else if (ScanState == EScanState::Processing)
		{
			if (SceneUpdateFuture.IsReady())
			{
				ProcessSceneUpdate(MoveTemp(*SceneUpdateFuture.Get()), DisplayTime, TrackingSpace);
				SceneUpdateFuture.Reset();
				UuidToLocateThisFrame = 0;
				// Avoid a frame rate dip by adding meshes over multiple frames after processing
				ScanState = EScanState::AddMeshesToScene;
			}
		}
		else if (ScanState == EScanState::AddMeshesToScene)
		{
			if (UuidsToLocate.Num() == 0 || LocatingScene.Handle() == XR_NULL_HANDLE)
			{
				ScanState = EScanState::Idle;
				return;
			}

			const float WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale();
			TrackedMeshHolder->StartMeshUpdates();

			for (int i = 0; i < UuidsToLocatePerFrame; i++)
			{
				const XrUuidMSFT& PlaneUuid = UuidsToLocate[UuidToLocateThisFrame];
				const FGuid PlaneGuid = XrUuidMSFTToFGuid(PlaneUuid);
				FPlaneUpdate& Plane = Planes.FindChecked(PlaneUuid);
				const FGuid& MeshGuid = Plane.MeshGuid;
				const auto& Location = Locations[UuidToLocateThisFrame];

				FOpenXRPlaneUpdate* PlaneUpdate = TrackedMeshHolder->AllocatePlaneUpdate(PlaneGuid);
				PlaneUpdate->Type = Plane.Type;
				PlaneUpdate->Extent = FVector(Plane.Extent);
				if (IsPoseValid(Location.flags))
				{
					PlaneUpdate->LocalToTrackingTransform = GetPlaneTransform(Location.pose, WorldToMetersScale);
				}
				else
				{
					// A location was not found, hide the mesh until it is located.
					PlaneUpdate->LocalToTrackingTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
				}

				if (MeshGuid.IsValid())
				{
					FOpenXRMeshUpdate* MeshUpdate = TrackedMeshHolder->AllocateMeshUpdate(MeshGuid);
					MeshUpdate->Type = Plane.Type;
					MeshUpdate->Vertices = MoveTemp(Plane.Vertices);
					MeshUpdate->Indices = MoveTemp(Plane.Indices);
					if (IsPoseValid(Location.flags))
					{
						MeshUpdate->LocalToTrackingTransform = ToFTransform(Location.pose, WorldToMetersScale);
					}
					else
					{
						// A location was not found, hide the mesh until it is located.
						MeshUpdate->LocalToTrackingTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
					}

					MeshUpdate->SpatialMeshUsageFlags =
						(EARSpatialMeshUsageFlags)((int32)EARSpatialMeshUsageFlags::Visible |
							(int32)EARSpatialMeshUsageFlags::Collision);
				}

				UuidToLocateThisFrame++;
				if (UuidToLocateThisFrame >= UuidsToLocate.Num())
				{
					UuidToLocateThisFrame = 0;
					ScanState = EScanState::Locating;
					break;
				}
			}

			TrackedMeshHolder->EndMeshUpdates();
		}

		UpdateObjectLocations(DisplayTime, TrackingSpace);
	}

	void FSceneUnderstandingBase::Stop()
	{
		bShouldStartSceneUnderstanding = false;
		ScanState = EScanState::Idle;
		LocatingScene.Reset();
		SceneObserver.Reset();
	}

	void FSceneUnderstandingBase::ComputeNewScene(XrTime DisplayTime)
	{
		SceneComputeInfo.requestedFeatureCount = static_cast<uint32_t>(ComputeFeatures.Num());
		SceneComputeInfo.requestedFeatures = ComputeFeatures.GetData();
		SceneComputeInfo.consistency = GetSceneComputeConsistency();
		SceneComputeInfo.bounds.space = ViewSpace.Handle();	  // scene bounds will be relative to view space
		SceneComputeInfo.bounds.time = DisplayTime;

		if (BoundHeight > 0)
		{
			SceneBox.pose = { {0, 0, 0, 1}, {0, 0, 0} };
			SceneBox.extents = { SphereBoundRadius, BoundHeight, SphereBoundRadius };
			SceneComputeInfo.bounds.boxCount = 1;
			SceneComputeInfo.bounds.boxes = &SceneBox;
		}
		else
		{
			SceneSphere.center = { 0, 0, 0 };
			SceneSphere.radius = SphereBoundRadius;
			SceneComputeInfo.bounds.sphereCount = 1;
			SceneComputeInfo.bounds.spheres = &SceneSphere;
		}

		XR_ENSURE(Ext.xrComputeNewSceneMSFT(SceneObserver.Handle(), &SceneComputeInfo));
	}
}
