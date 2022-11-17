// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandMeshPlugin.h"
#include "InputCoreTypes.h"
#include "OpenXRCore.h"
#include "IXRTrackingSystem.h"
#include "IOpenXRARModule.h"
#include "IOpenXRARTrackedGeometryHolder.h"
#include "ARSessionConfig.h"
#include "HeadMountedDisplayTypes.h"

#define LOCTEXT_NAMESPACE "FMicrosoftOpenXRModule"


namespace MicrosoftOpenXR
{
	FHandMeshPlugin::FHandState::FHandState()
	{
	}

	bool FHandMeshPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
		OutExtensions.Add(XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME);
		return true;
	}

	const void* FHandMeshPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		XrSystemHandTrackingMeshPropertiesMSFT HandMeshTrackingSystemProperties{ XR_TYPE_SYSTEM_HAND_TRACKING_MESH_PROPERTIES_MSFT };
		XrSystemHandTrackingPropertiesEXT HandTrackingSystemProperties{ XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT, &HandMeshTrackingSystemProperties };
		XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES, &HandTrackingSystemProperties };

		XR_ENSURE_MSFT(xrGetSystemProperties(InInstance, InSystem, &systemProperties));

		bool bHandMeshTrackingAvailable = HandTrackingSystemProperties.supportsHandTracking != XR_FALSE && HandMeshTrackingSystemProperties.supportsHandTrackingMesh != XR_FALSE;

		if (bHandMeshTrackingAvailable)
		{
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*)&xrCreateHandTrackerEXT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateHandMeshSpaceMSFT", (PFN_xrVoidFunction*)&xrCreateHandMeshSpaceMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrUpdateHandMeshMSFT", (PFN_xrVoidFunction*)&xrUpdateHandMeshMSFT));

			for (int i=0; i<HandCount; ++i)
			{
				FHandState& HandState = HandStates[i];

				HandState.IndicesCount = 0;
				HandState.VerticesCount = 0;

				HandState.VerticesMaxAmount = HandMeshTrackingSystemProperties.maxHandMeshVertexCount;
				HandState.IndicesMaxAmount = HandMeshTrackingSystemProperties.maxHandMeshIndexCount;

				HandState.Guid = FGuid::NewGuid();

				HandState.Vertices.resize(HandState.VerticesMaxAmount);
				HandState.Indices.resize(HandState.IndicesMaxAmount);
			}

			HandMeshStatus = EHandMeshStatus::Disabled;
		}
		else
		{
			HandMeshStatus = EHandMeshStatus::NotInitialised;
		}

		return InNext;
	}

	const void* FHandMeshPlugin::OnBeginSession(XrSession InSession, const void* InNext)
	{
		if (HandMeshStatus == EHandMeshStatus::NotInitialised)
		{
			return InNext;
		}
		static FName SystemName(TEXT("OpenXR"));
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
		{
			XRTrackingSystem = GEngine->XRSystem.Get();
		}
		else
		{
			HandMeshStatus = EHandMeshStatus::NotInitialised;
			return InNext;
		}

		if (IOpenXRARModule::IsAvailable())
		{
			TrackedMeshHolder = IOpenXRARModule::Get().GetTrackedMeshHolder();
		}
		else
		{
			HandMeshStatus = EHandMeshStatus::NotInitialised;
			return InNext;
		}

		for (int i = 0; i < HandCount; ++i)
		{
			FHandState& HandState = HandStates[i];

			XrHandTrackerCreateInfoEXT CreateInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
			CreateInfo.hand = XrHandEXT(XR_HAND_LEFT_EXT + i);
			CreateInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
			XR_ENSURE_MSFT(xrCreateHandTrackerEXT(InSession, &CreateInfo, &HandState.HandTracker));

			XrHandMeshSpaceCreateInfoMSFT SpaceCreateInfo{ XR_TYPE_HAND_MESH_SPACE_CREATE_INFO_MSFT };
			SpaceCreateInfo.handPoseType = XR_HAND_POSE_TYPE_TRACKED_MSFT;
			SpaceCreateInfo.poseInHandMeshSpace = ToXrPose(FTransform::Identity, XRTrackingSystem->GetWorldToMetersScale());
			XR_ENSURE_MSFT(xrCreateHandMeshSpaceMSFT(HandState.HandTracker, &SpaceCreateInfo, &HandState.Space));
		}

		return InNext;
	}

	void FHandMeshPlugin::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		check(IsInGameThread());

		if (HandMeshStatus <= EHandMeshStatus::Disabled)
		{
			return;
		}

		for (int i = 0; i < HandCount; ++i)
		{
			FHandState& HandState = HandStates[i];

			XrSpaceLocation SpaceLocation { XR_TYPE_SPACE_LOCATION };

			XR_ENSURE_MSFT(xrLocateSpace(HandState.Space, TrackingSpace, DisplayTime, &SpaceLocation));
			const XrSpaceLocationFlags ValidFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;

			if ((SpaceLocation.locationFlags & ValidFlags) == ValidFlags)
			{
				HandState.TrackingState = EARTrackingState::Tracking;
				HandState.LocalToTrackingTransform = ToFTransform(SpaceLocation.pose, XRTrackingSystem->GetWorldToMetersScale());

				XrHandMeshUpdateInfoMSFT HandMeshUpdateInfo { XR_TYPE_HAND_MESH_UPDATE_INFO_MSFT };
				HandMeshUpdateInfo.time = DisplayTime;
				HandMeshUpdateInfo.handPoseType = XR_HAND_POSE_TYPE_TRACKED_MSFT;

				XrHandMeshMSFT HandMesh { XR_TYPE_HAND_MESH_MSFT };
				HandMesh.indexBuffer.indexCapacityInput = HandState.Indices.size();
				HandMesh.indexBuffer.indices = HandState.Indices.data();

				HandMesh.vertexBuffer.vertexCapacityInput = HandState.Vertices.size();
				HandMesh.vertexBuffer.vertices = HandState.Vertices.data();

				XR_ENSURE_MSFT(xrUpdateHandMeshMSFT(HandState.HandTracker, &HandMeshUpdateInfo, &HandMesh));
				

				if (HandMesh.indexBufferChanged)
				{
					HandState.IndicesCount = HandMesh.indexBuffer.indexCountOutput;
					//changes in index buffer can't be independent, if index buffer changes, vertex buffers changes as well
				}

				if (HandMesh.vertexBufferChanged)
				{
					HandState.VerticesCount = HandMesh.vertexBuffer.vertexCountOutput;
				}
			}
			else
			{
				//tracking lost
				HandState.TrackingState = EARTrackingState::NotTracking;

				HandState.IndicesCount = 0;
				HandState.VerticesCount = 0;
			}
		}

		if (HandMeshStatus != EHandMeshStatus::EnabledTrackingGeometry)
		{
			return;
		}

		TrackedMeshHolder->StartMeshUpdates();
		for (int i = 0; i < HandCount; ++i)
		{
			FHandState& HandState = HandStates[i];

			FOpenXRMeshUpdate* MeshUpdate = TrackedMeshHolder->AllocateMeshUpdate(HandState.Guid);
			MeshUpdate->Type = EARObjectClassification::HandMesh;
			MeshUpdate->TrackingState = HandState.TrackingState;
			if (HandState.TrackingState != EARTrackingState::Tracking)
			{
				continue;
			}

			MeshUpdate->LocalToTrackingTransform = HandState.LocalToTrackingTransform;
			MeshUpdate->Indices.AddUninitialized(HandState.IndicesCount * 2);

			size_t TriangleCount = (size_t)HandState.IndicesCount / 3;

			auto DestIndices = MeshUpdate->Indices.GetData();
			auto RawIndices = HandState.Indices.data();

			for (size_t j = 0; j < TriangleCount; ++j)
			{
				//forward face
				DestIndices[0] = RawIndices[2];
				DestIndices[1] = RawIndices[1];
				DestIndices[2] = RawIndices[0];

				//backward face
				DestIndices[3] = RawIndices[0];
				DestIndices[4] = RawIndices[1];
				DestIndices[5] = RawIndices[2];

				DestIndices += 6;
				RawIndices += 3;
			}

			MeshUpdate->Vertices.AddUninitialized(HandState.VerticesCount);
			for (size_t j = 0; j < HandState.VerticesCount; ++j)
			{
				auto& destVert = MeshUpdate->Vertices[j];
				const auto& srcVert = HandState.Vertices[j];

				destVert = ToFVector3f(srcVert.position, XRTrackingSystem->GetWorldToMetersScale());
			}
		}
		TrackedMeshHolder->EndMeshUpdates();
	}

	void FHandMeshPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(IHandTracker::GetModularFeatureName(), static_cast<IHandTracker*>(this));
		IModularFeatures::Get().RegisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));
	}

	void FHandMeshPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(IHandTracker::GetModularFeatureName(), static_cast<IHandTracker*>(this));
		IModularFeatures::Get().UnregisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));
	}

	bool FHandMeshPlugin::Turn(EHandMeshStatus Mode)
	{
		check(IsInGameThread());

		if (HandMeshStatus == EHandMeshStatus::NotInitialised)
		{
			return false;
		}

		if (Mode == EHandMeshStatus::Disabled)
		{
			for (int i = 0; i < HandCount; ++i)
			{
				FHandState& HandState = HandStates[i];

				HandState.IndicesCount = 0;
				HandState.VerticesCount = 0;
				HandState.LocalToTrackingTransform.SetIdentity();
				HandState.TrackingState = EARTrackingState::NotTracking;

				if (HandMeshStatus == EHandMeshStatus::EnabledTrackingGeometry)
				{
					TrackedMeshHolder->RemoveMesh(HandState.Guid);
				}
			}
		}

		HandMeshStatus = Mode;

		return true;
	}

	bool FHandMeshPlugin::IsHandTrackingStateValid() const
	{
		if (HandMeshStatus <= EHandMeshStatus::Disabled)
		{
			return false;
		}

		for (int i = 0; i < HandCount; ++i)
		{
			const FHandState& HandState = HandStates[i];
			if (HandState.TrackingState == EARTrackingState::Tracking)
			{
				return true;
			}
		}

		return false;
	}

	bool FHandMeshPlugin::HasHandMeshData() const
	{
		if (HandMeshStatus <= EHandMeshStatus::Disabled)
		{
			return false;
		}

		for (int i = 0; i < HandCount; ++i)
		{
			const FHandState& HandState = HandStates[i];
			if (HandState.VerticesCount > 0)
			{
				return true;
			}
		}

		return false;
	}

	bool FHandMeshPlugin::GetHandMeshData(EControllerHand Hand, TArray<FVector>& OutVertices, TArray<FVector>& OutNormals, TArray<int32>& OutIndices, FTransform& OutHandMeshTransform) const
	{
		check(IsInGameThread());

		if (HandMeshStatus <= EHandMeshStatus::Disabled)
		{
			return false;
		}

		const FHandState* HandState = nullptr;
		if (Hand == EControllerHand::Left)
		{
			HandState = HandStates + Left;
		}
		else if (Hand == EControllerHand::Right)
		{
			HandState = HandStates + Right;
		}
		else
		{
			//do nothing for an unknown controller
			return false;
		}


		if (HandMeshStatus != EHandMeshStatus::EnabledXRVisualization)
		{
			OutIndices.Empty();
			OutVertices.Empty();
			OutNormals.Empty();
			OutHandMeshTransform = FTransform::Identity;

			return false;
		}

		//clear the data without deallocation
		OutIndices.Reset(HandState->IndicesCount * 2);
		OutVertices.Reset(HandState->VerticesCount);
		OutNormals.Reset(HandState->VerticesCount);

		if (HandState->TrackingState != EARTrackingState::Tracking)
		{
			return false;
		}

		OutHandMeshTransform = HandState->LocalToTrackingTransform * XRTrackingSystem->GetTrackingToWorldTransform();

		OutIndices.AddUninitialized(HandState->IndicesCount * 2);

		size_t TriangleCount = (size_t)HandState->IndicesCount / 3;

		auto DestIndices = OutIndices.GetData();
		auto RawIndices = HandState->Indices.data();

		for (size_t j = 0; j < TriangleCount; ++j)
		{
			//forward face
			DestIndices[0] = RawIndices[2];
			DestIndices[1] = RawIndices[1];
			DestIndices[2] = RawIndices[0];

			//backward face
			DestIndices[3] = RawIndices[0];
			DestIndices[4] = RawIndices[1];
			DestIndices[5] = RawIndices[2];

			DestIndices += 6;
			RawIndices += 3;
		}

		OutVertices.AddUninitialized(HandState->VerticesCount);
		OutNormals.AddUninitialized(HandState->VerticesCount);
		for (size_t j = 0; j < HandState->VerticesCount; ++j)
		{
			auto& destVert = OutVertices[j];
			auto& destNorm = OutNormals[j];
			const auto& srcVert = HandState->Vertices[j];

			destVert = ToFVector(srcVert.position, XRTrackingSystem->GetWorldToMetersScale());
			destNorm = -ToFVector(srcVert.normal);
		}

		return true;
	}

}	 // namespace MicrosoftOpenXR

#undef LOCTEXT_NAMESPACE