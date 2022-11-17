// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "OpenXRCommon.h"
#include "UniqueHandle.h"

inline bool operator==(const XrUuidMSFT& Lh, const XrUuidMSFT& Rh) noexcept
{
	return memcmp(&Rh, &Lh, sizeof(XrUuidMSFT)) == 0;
}

inline bool operator!=(const XrUuidMSFT& Lh, const XrUuidMSFT& Rh) noexcept
{
	return !(Lh == Rh);
}

// Allows XrUuidMSFT to be used as a key in TMap
inline uint32 GetTypeHash(const XrUuidMSFT& Uuid)
{
	static_assert(sizeof(XrUuidMSFT) == sizeof(uint64) * 2);
	const uint64* V = reinterpret_cast<const uint64*>(Uuid.bytes);
	return HashCombine(GetTypeHash(V[0]), GetTypeHash(V[1]));
}

namespace MicrosoftOpenXR
{
	struct ExtensionDispatchTable
	{
		PFN_xrEnumerateSceneComputeFeaturesMSFT xrEnumerateSceneComputeFeaturesMSFT;
		PFN_xrCreateSceneObserverMSFT xrCreateSceneObserverMSFT;
		PFN_xrDestroySceneObserverMSFT xrDestroySceneObserverMSFT;
		PFN_xrCreateSceneMSFT xrCreateSceneMSFT;
		PFN_xrDestroySceneMSFT xrDestroySceneMSFT;
		PFN_xrComputeNewSceneMSFT xrComputeNewSceneMSFT;
		PFN_xrGetSceneComputeStateMSFT xrGetSceneComputeStateMSFT;
		PFN_xrGetSceneComponentsMSFT xrGetSceneComponentsMSFT;
		PFN_xrLocateSceneComponentsMSFT xrLocateSceneComponentsMSFT;
		PFN_xrGetSceneMeshBuffersMSFT xrGetSceneMeshBuffersMSFT;
	};

	// Defines a move-only smart handle for XrSceneObserverMSFT.
	class FSceneObserverHandle : public MicrosoftOpenXR::TUniqueExtHandle<XrSceneObserverMSFT>
	{
	};
	// Defines a move-only smart handle for XrSceneMSFT.
	class FSceneHandle : public MicrosoftOpenXR::TUniqueExtHandle<XrSceneMSFT>
	{
	};

	inline constexpr bool IsPoseValid(XrSpaceLocationFlags LocationFlags)
	{
		constexpr XrSpaceLocationFlags PoseValidFlags =
			XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
		return (LocationFlags & PoseValidFlags) == PoseValidFlags;
	}

	inline FSceneObserverHandle CreateSceneObserver(const ExtensionDispatchTable& Ext, XrSession SessionHandle)
	{
		FSceneObserverHandle UniqueSceneObserverHandle;
		XrSceneObserverCreateInfoMSFT CreateInfo{XR_TYPE_SCENE_OBSERVER_CREATE_INFO_MSFT};
		XR_ENSURE(Ext.xrCreateSceneObserverMSFT(
			SessionHandle, &CreateInfo, UniqueSceneObserverHandle.Put(Ext.xrDestroySceneObserverMSFT)));
		return UniqueSceneObserverHandle;
	}

	inline FSceneHandle CreateScene(const ExtensionDispatchTable& Ext, XrSceneObserverMSFT SceneObserverHandle)
	{
		FSceneHandle UniqueSceneHandle;
		XrSceneCreateInfoMSFT CreateInfo{XR_TYPE_SCENE_CREATE_INFO_MSFT};
		XR_ENSURE(Ext.xrCreateSceneMSFT(SceneObserverHandle, &CreateInfo, UniqueSceneHandle.Put(Ext.xrDestroySceneMSFT)));
		return UniqueSceneHandle;
	}

	template <typename XrStruct, typename XrExtension>
	void InsertExtensionStruct(XrStruct& BaseStruct, XrExtension& ExtensionStruct)
	{
		ExtensionStruct.next = BaseStruct.next;
		BaseStruct.next = &ExtensionStruct;
	}

	inline FGuid XrUuidMSFTToFGuid(const XrUuidMSFT& Uuid)
	{
		static_assert(sizeof(FGuid) == sizeof(XrUuidMSFT));
		FGuid OutGuid;
		FMemory::Memcpy(&OutGuid, &Uuid, sizeof(FGuid));
		return OutGuid;
	}

	// Maps the OpenXR ObjectType to Unreal's object classification.
	inline EARObjectClassification GetObjectClassification(XrSceneObjectTypeMSFT ObjectType)
	{
		switch (ObjectType)
		{
			case XR_SCENE_OBJECT_TYPE_INFERRED_MSFT:
				return EARObjectClassification::Unknown;
			case XR_SCENE_OBJECT_TYPE_UNCATEGORIZED_MSFT:
				return EARObjectClassification::Unknown;
			case XR_SCENE_OBJECT_TYPE_BACKGROUND_MSFT:
				return EARObjectClassification::Unknown;
			case XR_SCENE_OBJECT_TYPE_WALL_MSFT:
				return EARObjectClassification::Wall;
			case XR_SCENE_OBJECT_TYPE_FLOOR_MSFT:
				return EARObjectClassification::Floor;
			case XR_SCENE_OBJECT_TYPE_CEILING_MSFT:
				return EARObjectClassification::Ceiling;
			case XR_SCENE_OBJECT_TYPE_PLATFORM_MSFT:
				return EARObjectClassification::Table;
			default:
				return EARObjectClassification::NotApplicable;
		}
	}

	inline XrSceneObjectTypeMSFT GetObjectType(const TMap<XrUuidMSFT, XrSceneObjectTypeMSFT>& ObjectTypeMap, const XrUuidMSFT& Uuid)
	{
		const auto* Value = ObjectTypeMap.Find(Uuid);
		return Value != nullptr ? *Value : XR_SCENE_OBJECT_TYPE_UNCATEGORIZED_MSFT;
	}

	// Maps a SceneObject's UUID to an ObjectType.
	inline TMap<XrUuidMSFT, XrSceneObjectTypeMSFT> GetObjectTypeMap(XrSceneMSFT SceneHandle, const ExtensionDispatchTable& Ext)
	{
		XrSceneComponentsGetInfoMSFT GetInfo{XR_TYPE_SCENE_COMPONENTS_GET_INFO_MSFT};
		GetInfo.componentType = XR_SCENE_COMPONENT_TYPE_OBJECT_MSFT;

		XrSceneComponentsMSFT SceneComponents{XR_TYPE_SCENE_COMPONENTS_MSFT};
		XR_ENSURE(Ext.xrGetSceneComponentsMSFT(SceneHandle, &GetInfo, &SceneComponents));
		const uint32_t Count = SceneComponents.componentCountOutput;

		TArray<XrSceneComponentMSFT> Components;
		Components.SetNum(Count);
		SceneComponents.componentCapacityInput = Count;
		SceneComponents.components = Components.GetData();

		TArray<XrSceneObjectMSFT> Objects;
		Objects.SetNum(Count);
		XrSceneObjectsMSFT sceneObjects{XR_TYPE_SCENE_OBJECTS_MSFT};
		sceneObjects.sceneObjectCount = Count;
		sceneObjects.sceneObjects = Objects.GetData();
		InsertExtensionStruct(SceneComponents, sceneObjects);

		XR_ENSURE(Ext.xrGetSceneComponentsMSFT(SceneHandle, &GetInfo, &SceneComponents));

		TMap<XrUuidMSFT, XrSceneObjectTypeMSFT> Map;
		Map.Reserve(Count);
		for (uint32_t Index = 0; Index < Count; Index++)
		{
			Map.Add(Components[Index].id, Objects[Index].objectType);
		}
		return Map;
	}

	inline void GetSceneVisibleMeshes(XrSceneMSFT SceneHandle, const ExtensionDispatchTable& Ext,
		const TArray<XrScenePlaneAlignmentTypeMSFT>& PlaneAlignmentFilters, TArray<XrSceneComponentMSFT>& Components,
		TArray<XrSceneMeshMSFT>& Meshes)
	{
		XrSceneComponentsGetInfoMSFT GetInfo{ XR_TYPE_SCENE_COMPONENTS_GET_INFO_MSFT };
		GetInfo.componentType = XR_SCENE_COMPONENT_TYPE_VISUAL_MESH_MSFT;
		XrScenePlaneAlignmentFilterInfoMSFT AlignmentFilter{ XR_TYPE_SCENE_PLANE_ALIGNMENT_FILTER_INFO_MSFT };
		if (PlaneAlignmentFilters.Num() > 0)
		{
			AlignmentFilter.alignmentCount = static_cast<uint32_t>(PlaneAlignmentFilters.Num());
			AlignmentFilter.alignments = PlaneAlignmentFilters.GetData();
			InsertExtensionStruct(GetInfo, AlignmentFilter);
		}
		XrSceneComponentsMSFT SceneComponents{ XR_TYPE_SCENE_COMPONENTS_MSFT };
		XR_ENSURE(Ext.xrGetSceneComponentsMSFT(SceneHandle, &GetInfo, &SceneComponents));
		const uint32_t Count = SceneComponents.componentCountOutput;
		Components.SetNum(Count);
		SceneComponents.componentCapacityInput = Count;
		SceneComponents.components = Components.GetData();
		Meshes.SetNum(Count);
		XrSceneMeshesMSFT SceneMeshes{ XR_TYPE_SCENE_MESHES_MSFT };
		SceneMeshes.sceneMeshCount = Count;
		SceneMeshes.sceneMeshes = Meshes.GetData();
		InsertExtensionStruct(SceneComponents, SceneMeshes);
		XR_ENSURE(Ext.xrGetSceneComponentsMSFT(SceneHandle, &GetInfo, &SceneComponents));
	}

	inline void GetScenePlanes(XrSceneMSFT SceneHandle, const ExtensionDispatchTable& Ext,
		const TArray<XrScenePlaneAlignmentTypeMSFT>& PlaneAlignmentFilters, TArray<XrSceneComponentMSFT>& Components,
		TArray<XrScenePlaneMSFT>& Planes)
	{
		XrSceneComponentsGetInfoMSFT GetInfo{XR_TYPE_SCENE_COMPONENTS_GET_INFO_MSFT};
		GetInfo.componentType = XR_SCENE_COMPONENT_TYPE_PLANE_MSFT;

		XrScenePlaneAlignmentFilterInfoMSFT AlignmentFilter{XR_TYPE_SCENE_PLANE_ALIGNMENT_FILTER_INFO_MSFT};
		if (PlaneAlignmentFilters.Num() > 0)
		{
			AlignmentFilter.alignmentCount = static_cast<uint32_t>(PlaneAlignmentFilters.Num());
			AlignmentFilter.alignments = PlaneAlignmentFilters.GetData();
			InsertExtensionStruct(GetInfo, AlignmentFilter);
		}

		XrSceneComponentsMSFT SceneComponents{XR_TYPE_SCENE_COMPONENTS_MSFT};
		XR_ENSURE(Ext.xrGetSceneComponentsMSFT(SceneHandle, &GetInfo, &SceneComponents));
		const uint32_t Count = SceneComponents.componentCountOutput;

		Components.SetNum(Count);
		SceneComponents.componentCapacityInput = Count;
		SceneComponents.components = Components.GetData();

		Planes.SetNum(Count);
		XrScenePlanesMSFT ScenePlanes{XR_TYPE_SCENE_PLANES_MSFT};
		ScenePlanes.scenePlaneCount = Count;
		ScenePlanes.scenePlanes = Planes.GetData();
		InsertExtensionStruct(SceneComponents, ScenePlanes);

		XR_ENSURE(Ext.xrGetSceneComponentsMSFT(SceneHandle, &GetInfo, &SceneComponents));
	}

	// Locate components given space and time.
	inline void LocateObjects(XrSceneMSFT SceneHandle, const ExtensionDispatchTable& Ext, XrSpace BaseSpace, XrTime Time,
		const TArray<XrUuidMSFT>& Identifiers, TArray<XrSceneComponentLocationMSFT>& Locations)
	{
		XrSceneComponentsLocateInfoMSFT LocateInfo{XR_TYPE_SCENE_COMPONENTS_LOCATE_INFO_MSFT};
		LocateInfo.baseSpace = BaseSpace;
		LocateInfo.time = Time;
		LocateInfo.componentIdCount = static_cast<uint32_t>(Identifiers.Num());
		LocateInfo.componentIds = Identifiers.GetData();

		Locations.SetNum(Identifiers.Num());
		XrSceneComponentLocationsMSFT ComponentLocations{XR_TYPE_SCENE_COMPONENT_LOCATIONS_MSFT};
		ComponentLocations.locationCount = static_cast<uint32_t>(Locations.Num());
		ComponentLocations.locations = Locations.GetData();

		XR_ENSURE(Ext.xrLocateSceneComponentsMSFT(SceneHandle, &LocateInfo, &ComponentLocations));
	}

	// Reads mesh vertices and 32-bit indices.
	inline void ReadMeshBuffers(XrSceneMSFT SceneHandle, const ExtensionDispatchTable& Ext, uint64_t MeshBufferId,
		TArray<XrVector3f>& VertexBuffer, TArray<uint32_t>& IndexBuffer)
	{
		XrSceneMeshBuffersGetInfoMSFT MeshGetInfo{XR_TYPE_SCENE_MESH_BUFFERS_GET_INFO_MSFT};
		MeshGetInfo.meshBufferId = MeshBufferId;

		XrSceneMeshBuffersMSFT MeshBuffers{XR_TYPE_SCENE_MESH_BUFFERS_MSFT};
		XrSceneMeshVertexBufferMSFT Vertices{XR_TYPE_SCENE_MESH_VERTEX_BUFFER_MSFT};
		XrSceneMeshIndicesUint32MSFT Indices{XR_TYPE_SCENE_MESH_INDICES_UINT32_MSFT};
		InsertExtensionStruct(MeshBuffers, Vertices);
		InsertExtensionStruct(MeshBuffers, Indices);
		XR_ENSURE(Ext.xrGetSceneMeshBuffersMSFT(SceneHandle, &MeshGetInfo, &MeshBuffers));

		VertexBuffer.SetNum(Vertices.vertexCountOutput);
		IndexBuffer.SetNum(Indices.indexCountOutput);
		Vertices.vertexCapacityInput = Vertices.vertexCountOutput;
		Indices.indexCapacityInput = Indices.indexCountOutput;
		Vertices.vertices = VertexBuffer.GetData();
		Indices.indices = IndexBuffer.GetData();
		XR_ENSURE(Ext.xrGetSceneMeshBuffersMSFT(SceneHandle, &MeshGetInfo, &MeshBuffers));
		VertexBuffer.SetNum(Vertices.vertexCountOutput);
		IndexBuffer.SetNum(Indices.indexCountOutput);
	}

	// Reads mesh vertices and 16-bit indices.
	inline void ReadMeshBuffers(XrSceneMSFT SceneHandle, const ExtensionDispatchTable& Ext, uint64_t MeshBufferId,
		TArray<XrVector3f>& VertexBuffer, TArray<uint16_t>& IndexBuffer)
	{
		XrSceneMeshBuffersGetInfoMSFT MeshGetInfo{XR_TYPE_SCENE_MESH_BUFFERS_GET_INFO_MSFT};
		MeshGetInfo.meshBufferId = MeshBufferId;

		XrSceneMeshBuffersMSFT MeshBuffers{XR_TYPE_SCENE_MESH_BUFFERS_MSFT};
		XrSceneMeshVertexBufferMSFT Vertices{XR_TYPE_SCENE_MESH_VERTEX_BUFFER_MSFT};
		XrSceneMeshIndicesUint16MSFT Indices{XR_TYPE_SCENE_MESH_INDICES_UINT16_MSFT};
		InsertExtensionStruct(MeshBuffers, Vertices);
		InsertExtensionStruct(MeshBuffers, Indices);
		XR_ENSURE(Ext.xrGetSceneMeshBuffersMSFT(SceneHandle, &MeshGetInfo, &MeshBuffers));

		VertexBuffer.SetNum(Vertices.vertexCountOutput);
		IndexBuffer.SetNum(Indices.indexCountOutput);
		Vertices.vertexCapacityInput = Vertices.vertexCountOutput;
		Indices.indexCapacityInput = Indices.indexCountOutput;
		Vertices.vertices = VertexBuffer.GetData();
		Indices.indices = IndexBuffer.GetData();
		XR_ENSURE(Ext.xrGetSceneMeshBuffersMSFT(SceneHandle, &MeshGetInfo, &MeshBuffers));
		VertexBuffer.SetNum(Vertices.vertexCountOutput);
		IndexBuffer.SetNum(Indices.indexCountOutput);
	}

}	 // namespace MicrosoftOpenXR
