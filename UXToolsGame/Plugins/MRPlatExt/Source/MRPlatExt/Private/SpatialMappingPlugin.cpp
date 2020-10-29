#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "SpatialMappingPlugin.h"
#include "ARBlueprintLibrary.h"

using namespace winrt::Windows::Perception::Spatial;
using namespace winrt::Windows::Perception::Spatial::Surfaces;

namespace MRPlatExt
{
	void FSpatialMappingPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));

		MeshOptions = SpatialSurfaceMeshOptions();
		MeshOptions.IncludeVertexNormals(false);
		MeshOptions.VertexPositionFormat(winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16G16B16A16IntNormalized);
		MeshOptions.TriangleIndexFormat(winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16UInt);
	}

	void FSpatialMappingPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));
		
		StopMeshObserver();
		AnchorLocalizationData.clear();
	}

	bool FSpatialMappingPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_PERCEPTION_ANCHOR_INTEROP_PREVIEW_EXTENSION_NAME);
		OutExtensions.Add(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME);
		return true;
	}

	const void* FSpatialMappingPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorFromPerceptionAnchorMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialAnchorFromPerceptionAnchorMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialAnchorSpaceMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialAnchorSpaceMSFT));
		XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrDestroySpatialAnchorMSFT", (PFN_xrVoidFunction*)&xrDestroySpatialAnchorMSFT));

		return InNext;
	}

	const void* FSpatialMappingPlugin::OnBeginSession(XrSession InSession, const void* InNext)
	{
		static FName SystemName(TEXT("OpenXR"));
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
		{
			XRTrackingSystem = GEngine->XRSystem.Get();
		}
		else
		{
			return InNext;
		}

		if (IOpenXRARModule::IsAvailable())
		{
			TrackedMeshHolder = IOpenXRARModule::Get().GetTrackedMeshHolder();
		}
		else
		{
			return InNext;
		}

		return InNext;
	}

	bool FSpatialMappingPlugin::FindMeshTransform(XrSpace AnchorSpace, XrTime DisplayTime, XrSpace TrackingSpace, FTransform MeshToCachedAnchorTransform, FTransform& Transform, bool& IsTracking)
	{
		auto Scale = XRTrackingSystem->GetWorldToMetersScale();

		IsTracking = false;

		XrSpaceLocation SpaceLocation{ XR_TYPE_SPACE_LOCATION };
		XrResult result = xrLocateSpace(AnchorSpace, TrackingSpace, DisplayTime, &SpaceLocation);
		if (XR_FAILED(result))
		{
			return false;
		}

		constexpr XrSpaceLocationFlags TrackingFlags = 
			XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT;
		IsTracking = ((SpaceLocation.locationFlags & TrackingFlags) == TrackingFlags);

		constexpr XrSpaceLocationFlags ValidFlags = 
			XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
		if ((SpaceLocation.locationFlags & ValidFlags) == ValidFlags)
		{
			FTransform LocalToTrackingTransform = ToFTransform(SpaceLocation.pose, Scale);
			Transform = MeshToCachedAnchorTransform * LocalToTrackingTransform;

			return true;
		}

		return false;
	}

	bool FSpatialMappingPlugin::GetTransformBetweenCoordinateSystems(SpatialCoordinateSystem From, SpatialCoordinateSystem To, FTransform& Transform)
	{
		auto transform = From.TryGetTransformTo(To);
		if (!transform)
		{
			return false;
		}

		auto meshToCached = transform.Value();
		Transform = WMRUtility::FromMixedRealityTransform(DirectX::XMLoadFloat4x4(&meshToCached), XRTrackingSystem->GetWorldToMetersScale());

		return true;
	}

	bool FSpatialMappingPlugin::LocateSpatialMeshInTrackingSpace(const FGuid& MeshID, SpatialCoordinateSystem MeshCoordinateSystem, XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace, FTransform& Transform)
	{
		bool IsTracking = false;

		// If a known anchor exists for this mesh, use it to locate the mesh transform.
		auto cachedAnchorDataForThisMesh = AnchorLocalizationData.find(MeshID);
		if (cachedAnchorDataForThisMesh != AnchorLocalizationData.end())
		{
			// This mesh could be localizing against another mesh's coordinate system, apply the offset when getting the mesh transform.
			FTransform MeshToCachedAnchorTransform;
			if (GetTransformBetweenCoordinateSystems(MeshCoordinateSystem, cachedAnchorDataForThisMesh->second->CoordinateSystem, MeshToCachedAnchorTransform)
				&& FindMeshTransform(cachedAnchorDataForThisMesh->second->AnchorSpace, DisplayTime, TrackingSpace, MeshToCachedAnchorTransform, Transform, IsTracking))
			{
				return true;
			}

			// If we failed to locate the mesh and are not tracking, return now.  The meshes will continue to be localized after tracking is regained.
			// However, if we do have tracking, we may have a new coordinate system for the mesh and should recreate it's localization data.
			if (!IsTracking)
			{
				return false;
			}
		}
		
		// If we get here with cached anchor data, The device is tracking, but we have failed to localize against the known cached data.
		// This likely happens when the mesh gets a new coordinate system.  Skip looking at existing localization data, and create new data instead.
		if (cachedAnchorDataForThisMesh == AnchorLocalizationData.end())
		{
			// Otherwise, attempt to locate the SpatialSurfaceMesh from all cached coordinate systems.
			for (const auto& data : AnchorLocalizationData)
			{
				// This mesh will be localizing against another mesh's coordinate system, apply the offset when getting the mesh transform.
				FTransform MeshToCachedAnchorTransform;
				if (!GetTransformBetweenCoordinateSystems(MeshCoordinateSystem, data.second->CoordinateSystem, MeshToCachedAnchorTransform))
				{
					continue;
				}

				if (FindMeshTransform(data.second->AnchorSpace, DisplayTime, TrackingSpace, MeshToCachedAnchorTransform, Transform, IsTracking))
				{
					// Add to the anchor localization data, so the next time this mesh is updated it will have a direct entry in the map.
					AnchorLocalizationData.insert({ MeshID, data.second });
					return true;
				}
			}
		}

		// If no cached anchor exists that can locate the mesh, create a new anchor.
		// Each spatial mapping mesh is positioned in its own space relative to its coordinate system.
		// Create a temporary anchor at this coordinate system to localize against the TrackingSpace.
		auto wmrAnchor = SpatialAnchor::TryCreateRelativeTo(MeshCoordinateSystem);
		if (wmrAnchor == nullptr)
		{
			// If the new anchor and all cached anchors cannot find the mesh, ignore it.
			return false;
		}

		XrSpatialAnchorMSFT Anchor;
		XrResult result = xrCreateSpatialAnchorFromPerceptionAnchorMSFT(InSession, winrt::get_unknown(wmrAnchor), &Anchor);
		if (XR_FAILED(result))
		{
			return false;
		}

		XrSpatialAnchorSpaceCreateInfoMSFT AnchorSpaceCreateDesc{ XR_TYPE_SPATIAL_ANCHOR_SPACE_CREATE_INFO_MSFT };
		AnchorSpaceCreateDesc.poseInAnchorSpace = ToXrPose(FTransform::Identity);
		AnchorSpaceCreateDesc.anchor = Anchor;

		bool MeshLocated = true;
		XrSpace AnchorSpace = XR_NULL_HANDLE;
		result = xrCreateSpatialAnchorSpaceMSFT(InSession, &AnchorSpaceCreateDesc, &AnchorSpace);
		if (XR_FAILED(result))
		{
			wmrAnchor = nullptr;
			xrDestroySpatialAnchorMSFT(Anchor);
			return false;
		}

		if (!FindMeshTransform(AnchorSpace, DisplayTime, TrackingSpace, FTransform::Identity, Transform, IsTracking))
		{
			// The mesh has not been located in tracking space, but this anchor space may still be locatable in the future.
			MeshLocated = false;
		}

		if (cachedAnchorDataForThisMesh == AnchorLocalizationData.end())
		{
			// Cache localization data to compare against future meshes.
			AnchorLocalizationData.insert({ MeshID, MakeShared<WMRAnchorLocalizationData>(AnchorSpace, MeshCoordinateSystem) });
		}
		else
		{
			// Overwrite existing anchor localization data.
			cachedAnchorDataForThisMesh->second = MakeShared<WMRAnchorLocalizationData>(AnchorSpace, MeshCoordinateSystem);
		}
		
		if (AnchorLocalizationData.size() > WarnAfterThisManyMeshes)
		{
			UE_LOG(LogHMD, Warning,
				TEXT("Spatial mapping has recognized %d spaces, performance may be impacted."),
				AnchorLocalizationData.size());
		}

		wmrAnchor = nullptr;
		xrDestroySpatialAnchorMSFT(Anchor);
		return MeshLocated;
	}

	void FSpatialMappingPlugin::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		std::map<FGuid, MeshLocalizationData>::iterator Mesh;

		{
			// Since UpdateDeviceLocations is performed on the game thread, update a single spatial mesh per frame. 
			// This prevents stalls when the number of spatial meshes is large.
			std::lock_guard<std::mutex> lock(MeshRefsLock);
			if (UniqueMeshes.empty())
			{
				return;
			}

			if (MeshToLocalizeThisFrame >= UniqueMeshes.size())
			{
				MeshToLocalizeThisFrame = 0;
			}

			Mesh = UniqueMeshes.begin();
			std::advance(Mesh, MeshToLocalizeThisFrame);
			MeshToLocalizeThisFrame++;

			if (Mesh == UniqueMeshes.end())
			{
				return;
			}
		}

		auto MeshUpdate = new FOpenXRMeshUpdate;
		MeshUpdate->TrackingState = EARTrackingState::Tracking;

		FTransform Transform;
		if (!LocateSpatialMeshInTrackingSpace(Mesh->first, Mesh->second.CoordinateSystem, InSession, DisplayTime, TrackingSpace, Transform))
		{
			Transform = Mesh->second.LastKnownTransform;
			MeshUpdate->TrackingState = EARTrackingState::NotTracking;
		}

		Mesh->second.LastKnownTrackingState = MeshUpdate->TrackingState;

		MeshUpdate->Id = Mesh->first;
		MeshUpdate->LocalToTrackingTransform = Transform;
		TrackedMeshHolder->ObjectUpdated(MeshUpdate);

		Mesh->second.LastKnownTransform = Transform;

		// Move the SurfaceObserver's bounding volume so it is always centered on the user.
		UpdateBoundingVolume();
	}

	IOpenXRCustomCaptureSupport* FSpatialMappingPlugin::GetCustomCaptureSupport(const EARCaptureType CaptureType)
	{
		if (CaptureType == EARCaptureType::SpatialMapping)
		{
			return this;
		}
		return nullptr;
	}

	bool FSpatialMappingPlugin::OnToggleARCapture(const bool On)
	{
		if (On)
		{
			if (!bGenerateSRMeshes)
			{
				UE_LOG(LogHMD, Log, TEXT("Must enable Generate Mesh Data From Tracked Geometry in the ARSessionConfig to use Spatial Mapping."));
				return false;
			}

			return StartMeshObserver();
		}
		else
		{
			StopMeshObserver();
		}

		return true;
	}

	// Perform a raycast against the spatial meshes.
	TArray<FARTraceResult> FSpatialMappingPlugin::OnLineTraceTrackedObjects(const TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe> ARCompositionComponent, const FVector Start, const FVector End, const EARLineTraceChannels TraceChannels)
	{
		TArray<FARTraceResult> Results;
		// Iterate over the tracked MeshGeometries rather than UniqueMeshes since the output TraceResult needs the MeshGeometry.
		TArray<UARMeshGeometry*> Meshes = UARBlueprintLibrary::GetAllGeometriesByClass<UARMeshGeometry>();

		std::lock_guard<std::mutex> lock(MeshRefsLock);
		for (UARMeshGeometry* Mesh : Meshes)
		{
			// Get the saved mesh data from the tracked mesh Guid.
			const auto& it = UniqueMeshes.find(Mesh->UniqueId);
			if (it != UniqueMeshes.end())
			{
				FVector HitPoint, HitNormal;
				float HitDistance;
				if (it->second.CollisionInfo.Collides(Start, End, Mesh->GetLocalToWorldTransform(), HitPoint, HitNormal, HitDistance))
				{
					// Append a hit.  The calling function will then sort by HitDistance.
					Results.Add(FARTraceResult(ARCompositionComponent,
						HitDistance, 
						TraceChannels, 
						FTransform(HitNormal.ToOrientationQuat(), HitPoint),
						Mesh));
				}
			}
		}

		return Results;
	}

	void FSpatialMappingPlugin::OnStartARSession(class UARSessionConfig* SessionConfig)
	{
		GConfig->GetFloat(TEXT("/Script/HoloLensPlatformEditor.HoloLensTargetSettings"), TEXT("SpatialMeshingVolumeSize"), VolumeSize, GEngineIni);
		GConfig->GetFloat(TEXT("/Script/HoloLensPlatformEditor.HoloLensTargetSettings"), TEXT("MaxTrianglesPerCubicMeter"), TriangleDensity, GEngineIni);

		bGenerateSRMeshes = SessionConfig->bGenerateMeshDataFromTrackedGeometry;
	}

	bool FSpatialMappingPlugin::StartMeshObserver()
	{
		if (TrackedMeshHolder == nullptr)
		{
			return false;
		}
		
		std::lock_guard<std::mutex> lock(MeshRefsLock);
		if (SurfaceObserver != nullptr ||
			(RequestAccessOperation != nullptr &&
				RequestAccessOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Started))
		{
			UE_LOG(LogHMD, Log, TEXT("Attempting to call StartMeshObserver more than once."));
			return true;
		}

		if (!SpatialSurfaceObserver::IsSupported())
		{
			UE_LOG(LogHMD, Warning, TEXT("SpatialSurfaceObserver::IsSupported() returned false. No updates will occur."));
			return false;
		}

		RequestAccessOperation = SpatialSurfaceObserver::RequestAccessAsync();
		RequestAccessOperation.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
		{
			if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed &&
				asyncInfo.GetResults() == SpatialPerceptionAccessStatus::Allowed)
			{
				SurfaceObserver = SpatialSurfaceObserver();
				if (SurfaceObserver != nullptr)
				{
					UpdateBoundingVolume();

					check(!OnChangeEventToken);
					OnChangeEventToken = SurfaceObserver.ObservedSurfacesChanged([this](auto&& sender, auto&& obj) { OnSurfacesChanged(sender, obj); });
				}
				else
				{
					UE_LOG(LogHMD, Warning, TEXT("Failed to create spatial observer. No updates will occur."));
				}
			}
			else
			{
				UE_LOG(LogHMD, Warning, TEXT("User denied permission for spatial mapping. No updates will occur."));
			}
		});

		return true;
	}

	void FSpatialMappingPlugin::StopMeshObserver()
	{
		std::lock_guard<std::mutex> lock(MeshRefsLock);

		if (SurfaceObserver != nullptr)
		{
			SurfaceObserver.ObservedSurfacesChanged(OnChangeEventToken);
			OnChangeEventToken = winrt::event_token();

			SurfaceObserver = nullptr;
		}

		if (RequestAccessOperation != nullptr && RequestAccessOperation.Status() != winrt::Windows::Foundation::AsyncStatus::Completed)
		{
			RequestAccessOperation.Cancel();
		}
		RequestAccessOperation = nullptr;
	}

	void FSpatialMappingPlugin::UpdateBoundingVolume()
	{
		if (SurfaceObserver == nullptr)
		{
			return;
		}

		SpatialBoundingBox AABB =
		{
			{ 0.0f, 0.0f, 0.0f },
			{ VolumeSize, VolumeSize, VolumeSize }
		};

		const auto coordinateSystem = SpatialLocator::GetDefault().CreateStationaryFrameOfReferenceAtCurrentLocation().CoordinateSystem();
		if (coordinateSystem == nullptr)
		{
			return;
		}

		SpatialBoundingVolume BoundingVolume = SpatialBoundingVolume::FromBox(coordinateSystem, AABB);
		if (BoundingVolume == nullptr)
		{
			return;
		}

		SurfaceObserver.SetBoundingVolume(BoundingVolume);
	}

	void FSpatialMappingPlugin::CopyMeshData(FOpenXRMeshUpdate* MeshUpdate, SpatialSurfaceMesh SurfaceMesh)
	{
		int VertexCount = SurfaceMesh.VertexPositions().ElementCount();
		int IndexCount = SurfaceMesh.TriangleIndices().ElementCount();
		MeshUpdate->Vertices.AddUninitialized(VertexCount);
		MeshUpdate->Indices.AddUninitialized(IndexCount);

		FVector MeshScale = WMRUtility::FromMixedRealityScale(SurfaceMesh.VertexPositionScale());

		DirectX::PackedVector::XMSHORTN4* RawVertices = reinterpret_cast<DirectX::PackedVector::XMSHORTN4*>(SurfaceMesh.VertexPositions().Data().data());
		check(SurfaceMesh.TriangleIndices().Format() == winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16UInt);
		unsigned short* RawIndices = reinterpret_cast<unsigned short*>(SurfaceMesh.TriangleIndices().Data().data());

		float Scale = XRTrackingSystem->GetWorldToMetersScale();
		for (int Index = 0; Index < VertexCount; Index++)
		{
			// Match alignment with MeshOptions->VertexPositionFormat
			DirectX::PackedVector::XMSHORTN4 packedSource = RawVertices[Index];
			DirectX::XMVECTOR Source = DirectX::PackedVector::XMLoadShortN4(&packedSource);

			MeshUpdate->Vertices[Index] = WMRUtility::FromXMVectorTranslation(Source, Scale) * MeshScale;
		}

		int TriangleCount = IndexCount / 3;
		MRMESH_INDEX_TYPE* DestIndices = (MRMESH_INDEX_TYPE*)MeshUpdate->Indices.GetData();

		// Reverse triangle order
		for (int Index = 0; Index < TriangleCount; Index++)
		{
			DestIndices[0] = RawIndices[2];
			DestIndices[1] = RawIndices[1];
			DestIndices[2] = RawIndices[0];
			DestIndices += 3;
			RawIndices += 3;
		}
	}

	void FSpatialMappingPlugin::OnSurfacesChanged(SpatialSurfaceObserver Observer, winrt::Windows::Foundation::IInspectable)
	{
		auto SurfaceCollection = Observer.GetObservedSurfaces();
		std::vector<FGuid> ObservedSurfacesThisUpdate;

		ObservedSurfacesThisUpdate.clear();
		for (auto Iter = SurfaceCollection.First(); Iter.HasCurrent(); Iter.MoveNext())
		{
			auto KVPair = Iter.Current();

			winrt::guid Id = KVPair.Key();
			FGuid MeshId = WMRUtility::GUIDToFGuid(Id);
			ObservedSurfacesThisUpdate.push_back(MeshId);
			SpatialSurfaceInfo SurfaceInfo = KVPair.Value();

			winrt::Windows::Foundation::IAsyncOperation<SpatialSurfaceMesh> ComputeMeshAsyncOperation =
				SurfaceInfo.TryComputeLatestMeshAsync(TriangleDensity, MeshOptions);
			if (ComputeMeshAsyncOperation == nullptr)
			{
				continue;
			}

			ComputeMeshAsyncOperation.Completed([this, MeshId](winrt::Windows::Foundation::IAsyncOperation<SpatialSurfaceMesh> asyncOperation, winrt::Windows::Foundation::AsyncStatus status)
			{
				if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
				{
					auto SurfaceMesh = asyncOperation.GetResults();
					if (SurfaceMesh != nullptr)
					{
						TrackedMeshHolder->StartMeshUpdates();
						
						FOpenXRMeshUpdate* MeshUpdate = TrackedMeshHolder->AllocateMeshUpdate(MeshId);
						MeshUpdate->Type = EARObjectClassification::World;
						CopyMeshData(MeshUpdate, SurfaceMesh);

						const auto& it = UniqueMeshes.find(MeshId);
						if (it != UniqueMeshes.end())
						{
							// If a mesh entry already existed for this spatial mesh, use the last known transform for the first update to keep it close to where it was previously.
							MeshUpdate->LocalToTrackingTransform = it->second.LastKnownTransform;
							MeshUpdate->TrackingState = it->second.LastKnownTrackingState;

							// Update the cached mesh data
							std::lock_guard<std::mutex> lock(MeshRefsLock);
							it->second.CoordinateSystem = SurfaceMesh.CoordinateSystem();
							it->second.CollisionInfo.UpdateVertices(MeshUpdate->Vertices, MeshUpdate->Indices);
						}
						else
						{
							// This is the first time observing this mesh
							std::lock_guard<std::mutex> lock(MeshRefsLock);
							// Guarantee the mesh is not seen until a valid transform has been found.
							FTransform TempTransform = FTransform::Identity;
							TempTransform.SetScale3D(FVector::ZeroVector);
							
							// Don't set the tracking state until the LocalToTrackingTransform is identified in UpdateDeviceLocations.
							MeshUpdate->TrackingState = EARTrackingState::NotTracking;

							UniqueMeshes.insert({ MeshId, MeshLocalizationData { 
								TempTransform, 
								EARTrackingState::NotTracking, 
								SurfaceMesh.CoordinateSystem(), 
								TrackedGeometryCollision(MeshUpdate->Vertices, MeshUpdate->Indices) 
								} });

							MeshUpdate->LocalToTrackingTransform = TempTransform;
						}

						TrackedMeshHolder->EndMeshUpdates();
					}
				}
			});
		}

		// Check for removed meshes
		if (UniqueMeshes.size() != ObservedSurfacesThisUpdate.size())
		{
			std::lock_guard<std::mutex> lock(MeshRefsLock);

			std::vector<FGuid> MeshesToRemove = std::vector<FGuid>();
			for (const auto& Mesh : UniqueMeshes)
			{
				if (std::find(ObservedSurfacesThisUpdate.begin(), 
					ObservedSurfacesThisUpdate.end(), Mesh.first) == ObservedSurfacesThisUpdate.end())
				{
					// Cached mesh was not found, it has been disposed.
					MeshesToRemove.push_back(Mesh.first);

					TrackedMeshHolder->RemoveMesh(Mesh.first);
				}
			}

			for (const auto& MeshId : MeshesToRemove)
			{
				UniqueMeshes.erase(MeshId);
			}
		}
	}
}
#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS