// Copyright (c) 2022 Microsoft Corporation.
// Licensed under the MIT License.

#include "AzureObjectAnchorsPlugin.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.Storage.h"
#include "winrt/Windows.Storage.Streams.h"

namespace WF = winrt::Windows::Foundation;
namespace WFN = WF::Numerics;

namespace MicrosoftOpenXR
{
	void FAzureObjectAnchorsPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

		const FString PluginBaseDir = IPluginManager::Get().FindPlugin("MicrosoftOpenXR")->GetBaseDir();
		FString PackageRelativePath = PluginBaseDir / THIRDPARTY_BINARY_SUBFOLDER;

		// On HoloLens, DLLs must be loaded relative to the package with no ".."'s in the path. 
		// If using FPlatformProcess::PushDLLDirectory, the library path must be made relative to the RootDir.
#if PLATFORM_HOLOLENS
		FPaths::MakePathRelativeTo(PackageRelativePath, *(FPaths::RootDir() + TEXT("/")));
#endif
		FString Binaries[] = {
			"ObjectTrackerApi.dll",
			"ObjectTrackerDiagnostics.dll",
			"VolumeFusionAPI.dll",
			"ObjectTrackerFusion.dll",
			"ObjectTrackerRefinement.dll",
			"Microsoft.Azure.ObjectAnchors.dll"
		};

		FPlatformProcess::PushDllDirectory(*PackageRelativePath);
		for (FString Binary : Binaries)
		{
			if (!FPlatformProcess::GetDllHandle(*Binary))
			{
				UE_LOG(LogAOA, Warning, TEXT("Dll \'%s\' can't be loaded from \'%s\'"), *Binary, *PackageRelativePath);
			}
		}
		FPlatformProcess::PopDllDirectory(*PackageRelativePath);
	}

	void FAzureObjectAnchorsPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);

		if (ObjectAnchorObserver)
		{
			ObjectAnchorObserver.Close();
			ObjectAnchorObserver = nullptr;
		}
	}

	FAzureObjectAnchorsPlugin::~FAzureObjectAnchorsPlugin()
	{
		if (DetectTask)
		{
			DetectTask.Cancel();
		}
	}

	bool FAzureObjectAnchorsPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SPATIAL_GRAPH_BRIDGE_EXTENSION_NAME);
		return true;
	}

	const void* FAzureObjectAnchorsPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{	
		// First check for the correct spatial graph bridge extension version.
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction*)&xrEnumerateInstanceExtensionProperties));
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialGraphNodeSpaceMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialGraphNodeSpaceMSFT));

		uint32_t ExtensionsCount = 0;
		if (XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &ExtensionsCount, nullptr)))
		{
			TArray<XrExtensionProperties> Properties;
			Properties.SetNum(ExtensionsCount);
			for (auto& Prop : Properties)
			{
				Prop = XrExtensionProperties{ XR_TYPE_EXTENSION_PROPERTIES };
			}

			if (XR_ENSURE_MSFT(xrEnumerateInstanceExtensionProperties(nullptr, ExtensionsCount, &ExtensionsCount, Properties.GetData())))
			{
				for (const XrExtensionProperties& Prop : Properties)
				{
					if (strcmp(Prop.extensionName, XR_MSFT_SPATIAL_GRAPH_BRIDGE_EXTENSION_NAME) == 0)
					{
						CanUseSpatialGraphExtension = Prop.extensionVersion >= 2;
						break;
					}
				}
			}
		}

		if (CanUseSpatialGraphExtension)
		{
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrTryCreateSpatialGraphStaticNodeBindingMSFT", (PFN_xrVoidFunction*)&xrTryCreateSpatialGraphStaticNodeBindingMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetSpatialGraphNodeBindingPropertiesMSFT", (PFN_xrVoidFunction*)&xrGetSpatialGraphNodeBindingPropertiesMSFT));
		}
		else
		{
			UE_LOG(LogAOA, Warning, TEXT("Current OpenXR runtime does not support Azure Object Anchors.  Requires XR_MSFT_SPATIAL_GRAPH_BRIDGE version 2."));
		}

		if (IOpenXRARModule::IsAvailable())
		{
			TrackedMeshHolder = IOpenXRARModule::Get().GetTrackedMeshHolder();
		}

		return InNext;
	}

	const void* FAzureObjectAnchorsPlugin::OnBeginSession(XrSession InSession, const void* InNext)
	{
		Session = InSession;
		WorldToMetersScale = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(GWorld);

		return InNext;
	}

	void FAzureObjectAnchorsPlugin::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		{
			FScopeLock Lock(&MeshComponentLock);
			if (IsAzureObjectAnchorsStartDesired)
			{
				// Azure Object Anchors was initialized too early, attempt to start it now.
				InitAzureObjectAnchors(AzureObjectAnchorConfiguration);
				return;
			}

			if (!CanUseSpatialGraphExtension ||
				AOAMap.Num() == 0 ||
				ObjectAnchorObserver == nullptr ||
				IsCurrentlyDetecting)
			{
				return;
			}
		}

		if (OriginCoordinateSystemTrackingSpace != TrackingSpace
			|| !FrameOfReferenceLocated)
		{
			FScopeLock Lock(&MeshComponentLock);
			for (auto& data : AOAMap)
			{
				data.Value.ResetObjectInstance(TrackedMeshHolder);
			}

			XrSpatialGraphStaticNodeBindingCreateInfoMSFT SpatialGraphStaticNodeBindingCreateInfo
			{
				XR_TYPE_SPATIAL_GRAPH_STATIC_NODE_BINDING_CREATE_INFO_MSFT,
				nullptr,
				TrackingSpace,
				ToXrPose(FTransform::Identity),
				DisplayTime
			};

			XrSpatialGraphNodeBindingMSFT NodeBinding;
			if (XR_SUCCEEDED(xrTryCreateSpatialGraphStaticNodeBindingMSFT(InSession, &SpatialGraphStaticNodeBindingCreateInfo, &NodeBinding)))
			{
				if (NodeBinding == XR_NULL_HANDLE)
				{
					// If the NodeBinding fails to be created, try again next frame.
					UE_LOG(LogAOA, Warning, TEXT("SpatialGraphStaticNodeBinding was not created."));
					return;
				}

				XrSpatialGraphNodeBindingPropertiesGetInfoMSFT PropertiesGetInfo{ XR_TYPE_SPATIAL_GRAPH_NODE_BINDING_PROPERTIES_GET_INFO_MSFT };
				XrSpatialGraphNodeBindingPropertiesMSFT NodeProperties{ XR_TYPE_SPATIAL_GRAPH_NODE_BINDING_PROPERTIES_MSFT };

				if (XR_SUCCEEDED(xrGetSpatialGraphNodeBindingPropertiesMSFT(NodeBinding, &PropertiesGetInfo, &NodeProperties)))
				{
					const XrQuaternionf& Rot = NodeProperties.poseInNodeSpace.orientation;
					const XrVector3f& Pos = NodeProperties.poseInNodeSpace.position;
					const WFN::float4x4 CoordinateSystemToNodeTransform =
						WFN::make_float4x4_from_quaternion(WFN::quaternion(Rot.x, Rot.y, Rot.z, Rot.w)) *
						WFN::make_float4x4_translation(WFN::float3(Pos.x, Pos.y, Pos.z));

					OriginCoordinateSystem = { 
						winrt::guid(reinterpret_cast<GUID const&>(NodeProperties.nodeId)),
						CoordinateSystemToNodeTransform
					};
					OriginCoordinateSystemTrackingSpace = TrackingSpace;

					FrameOfReferenceLocated = true;
				}
			}
		}

		if (!FrameOfReferenceLocated)
		{
			return;
		}

		// Locate object anchors that have changed.
		{
			FScopeLock Lock(&MeshComponentLock);
			for (auto& data : AOAMap)
			{
				if (data.Value.HasChanged)
				{
					data.Value.HasChanged = false;

					// If the Object instance has closed or cannot be found, reset it.
					if (data.Value.ObjectInstance == nullptr ||
						data.Value.ObjectInstance.TryGetCurrentState() == nullptr)
					{
						data.Value.ResetObjectInstance(TrackedMeshHolder);
						continue;
					}

					TrackedMeshHolder->StartMeshUpdates();

					FOpenXRMeshUpdate* MeshUpdate = TrackedMeshHolder->AllocateMeshUpdate(data.Key);
					MeshUpdate->Type = EARObjectClassification::SceneObject;

					MeshUpdate->Indices = data.Value.Indices;
					MeshUpdate->Vertices = data.Value.Vertices;

					XrSpaceLocation Location{ XR_TYPE_SPACE_LOCATION };
					xrLocateSpace(data.Value.Space, TrackingSpace, DisplayTime, &Location);
					const XrSpaceLocationFlags ValidFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
					if ((Location.locationFlags & ValidFlags) == ValidFlags)
					{
						MeshUpdate->TrackingState = EARTrackingState::Tracking;
						MeshUpdate->LocalToTrackingTransform = ToFTransform(Location.pose, WorldToMetersScale);
					}
					else
					{
						MeshUpdate->TrackingState = EARTrackingState::NotTracking;
						MeshUpdate->LocalToTrackingTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
					}

					MeshUpdate->SpatialMeshUsageFlags =
						(EARSpatialMeshUsageFlags)((int32)EARSpatialMeshUsageFlags::Visible |
							(int32)EARSpatialMeshUsageFlags::Collision);

					TrackedMeshHolder->EndMeshUpdates();
				}
			}
		}

		IsCurrentlyDetecting = true;

		Queries.clear();
		{
			FScopeLock Lock(&MeshComponentLock);
			for (auto& data : AOAMap)
			{	
				data.Value.ResetStaleObjectInstance(TrackedMeshHolder);
				if (!data.Value.IsCurrentlyBeingTracked())
				{
					OA::ObjectQuery Query = OA::ObjectQuery(data.Value.ObjectModel);

					if (SearchAreas.empty())
					{
						FXRHMDData HMDData;
						UHeadMountedDisplayFunctionLibrary::GetHMDData(GWorld, HMDData);
						FVector HMDPosTrackingSpace = UHeadMountedDisplayFunctionLibrary::GetTrackingToWorldTransform(GWorld).Inverse().TransformPosition(HMDData.Position);
						DirectX::XMFLOAT3 HeadPos = WMRUtility::ToMixedRealityVector(HMDPosTrackingSpace / WorldToMetersScale);

						OA::SpatialGraph::SpatialSphere sphere =
						{
							WFN::float3(HeadPos.x, HeadPos.y, HeadPos.z),
							AzureObjectAnchorConfiguration.SearchRadius / WorldToMetersScale
						};

						OA::ObjectSearchArea SearchArea = OA::ObjectSearchArea::FromSphere(OriginCoordinateSystem, sphere);
						Query.SearchAreas().Append(SearchArea);
					}
					else
					{
						for (OA::SpatialGraph::SpatialSphere sphere : SearchAreas)
						{
							OA::ObjectSearchArea SearchArea = OA::ObjectSearchArea::FromSphere(OriginCoordinateSystem, sphere);
							Query.SearchAreas().Append(SearchArea);
						}
					}

					if (AzureObjectAnchorConfiguration.QueryModifiers != nullptr)
					{
						if (AzureObjectAnchorConfiguration.QueryModifiers->bUseExpectedMaxVerticalOrientationInDegrees)
						{
							Query.ExpectedMaxVerticalOrientationInDegrees(AzureObjectAnchorConfiguration.QueryModifiers->ExpectedMaxVerticalOrientationInDegrees);
						}
						if (AzureObjectAnchorConfiguration.QueryModifiers->bUseExpectedToBeStandingOnGroundPlane)
						{
							Query.IsExpectedToBeStandingOnGroundPlane(AzureObjectAnchorConfiguration.QueryModifiers->ExpectedToBeStandingOnGroundPlane);
						}
						if (AzureObjectAnchorConfiguration.QueryModifiers->bUseMaxScaleChange)
						{
							Query.MaxScaleChange(AzureObjectAnchorConfiguration.QueryModifiers->MaxScaleChange);
						}
						if (AzureObjectAnchorConfiguration.QueryModifiers->bUseMinSurfaceCoverage)
						{
							Query.MinSurfaceCoverage(AzureObjectAnchorConfiguration.QueryModifiers->MinSurfaceCoverage);
						}
					}
					Queries.emplace_back(std::move(Query));
				}
			}
		}

		if (Queries.size() == 0)
		{
			IsCurrentlyDetecting = false;
			return;
		}

		// Try to find any currently untracked object models.
		TWeakPtr<FAzureObjectAnchorsPlugin> WeakSelf = SharedThis(this);
		DetectTask = ObjectAnchorObserver.DetectAsync(Queries);
		DetectTask.Completed([WeakSelf](
			WF::IAsyncOperation<WF::Collections::IVector<OA::ObjectInstance>> AsyncOp,
			WF::AsyncStatus Status)
		{
			if (TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin())
			{
				if (Status != WF::AsyncStatus::Completed)
				{
					UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors DetectAsync failed."));
					
					Self->IsCurrentlyDetecting = false;
					return;
				}

				WF::Collections::IVector<OA::ObjectInstance> Objects = AsyncOp.GetResults();
				for (OA::ObjectInstance Object : Objects)
				{
					if (Object.TryGetCurrentState() != nullptr)
					{
						Object.Mode(Self->AzureObjectAnchorConfiguration.TrackingMode == EObjectInstanceTrackingMode::LowLatencyCoarsePosition ?
							OA::ObjectInstanceTrackingMode::LowLatencyCoarsePosition :
							OA::ObjectInstanceTrackingMode::HighLatencyAccuratePosition
						);

						// Create a space with the initial state in case the changed event never fires
						Self->UpdateSpatialGraphInfoForObjectInstance(Object);

						Self->AOAMap[WMRUtility::GUIDToFGuid(Object.ModelId())].ObjectInstanceChangedEventToken =
							Object.Changed([WeakSelf](winrt::Windows::Foundation::IInspectable sender, OA::ObjectInstanceChangedEventArgs args)
						{
							if (TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin())
							{
								auto Object = sender.as<OA::ObjectInstance>();
								Self->UpdateSpatialGraphInfoForObjectInstance(Object);
							}
						});
					}
					else
					{
						Object.Close();
					}
				}

				Self->IsCurrentlyDetecting = false;
			}
		});
	}

	void FAzureObjectAnchorsPlugin::UpdateSpatialGraphInfoForObjectInstance(OA::ObjectInstance Object)
	{
		FScopeLock Lock(&MeshComponentLock);

		FGuid guid = WMRUtility::GUIDToFGuid(Object.ModelId());

		AOAContext& AOAEntry = AOAMap[guid];
		AOAEntry.ObjectInstance = Object;
		auto State = Object.TryGetCurrentState();
		if (State != nullptr)
		{
			OA::ObjectInstanceState CurrentState = State.as<OA::ObjectInstanceState>();

			// Create a new XrSpace for this node if necessary.
			if (AOAEntry.Space == XR_NULL_HANDLE
				|| AOAEntry.SpatialGraphNodeId != CurrentState.Center.NodeId)
			{
				if (AOAEntry.Space != XR_NULL_HANDLE)
				{
					xrDestroySpace(AOAEntry.Space);
				}

				XrSpatialGraphNodeSpaceCreateInfoMSFT SpatialGraphNodeSpaceCreateInfo{ XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT };
				SpatialGraphNodeSpaceCreateInfo.nodeType = XR_SPATIAL_GRAPH_NODE_TYPE_STATIC_MSFT;
				SpatialGraphNodeSpaceCreateInfo.pose = XrPosef{
					XrQuaternionf
					{
						CurrentState.Center.Orientation.x,
						CurrentState.Center.Orientation.y,
						CurrentState.Center.Orientation.z,
						CurrentState.Center.Orientation.w
					},
					XrVector3f
					{
						CurrentState.Center.Position.x,
						CurrentState.Center.Position.y,
						CurrentState.Center.Position.z
					}
				};

				FMemory::Memcpy(&SpatialGraphNodeSpaceCreateInfo.nodeId, &CurrentState.Center.NodeId, sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId));

				XrSpace Space;
				if (XR_SUCCEEDED(xrCreateSpatialGraphNodeSpaceMSFT(Session, &SpatialGraphNodeSpaceCreateInfo, &Space)))
				{
					// If this fails, the next object update will try again.
					AOAEntry.Space = Space;
				}
			}

			AOAEntry.SpatialGraphNodeId = CurrentState.Center.NodeId;
			AOAEntry.LastKnownState = CurrentState;
			AOAEntry.HasChanged = true;
		}
	}

	void FAzureObjectAnchorsPlugin::OnStartARSession(class UARSessionConfig* SessionConfig)
	{
		// ARTrackedGeometry requires a valid ARSession to know what class to use when initializing added geometry.
		ARSessionStarted = true;
	}

	bool FAzureObjectAnchorsPlugin::OnToggleARCapture(const bool bOnOff)
	{
		// Toggling "on" is a no-op, because this override does not support adding a session configuration.
		// InitAzureObjectAnchors must be called to start the session.

		if (!bOnOff)
		{
			StopAzureObjectAnchors();
		}

		return true;
	}

	void FAzureObjectAnchorsPlugin::InitAzureObjectAnchors(FAzureObjectAnchorSessionConfiguration AOAConfiguration)
	{
		if (UMicrosoftOpenXRFunctionLibrary::IsRemoting())
		{
			UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors is not supported over remoting."));
			return;
		}

		if (!OA::ObjectObserver::IsSupported())
		{
			UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors is not supported on this device."));
			return;
		}

		AzureObjectAnchorConfiguration = AOAConfiguration;

		FScopeLock Lock(&MeshComponentLock);
		if (!ARSessionStarted || TrackedMeshHolder == nullptr)
		{
			// Azure Object Anchors is not ready to start, 
			// Set this flag to start when ready.
			IsAzureObjectAnchorsStartDesired = true;
			return;
		}

		// Ready to start, reset flag.
		IsAzureObjectAnchorsStartDesired = false;

		if (ObjectAnchorObserver != nullptr)
		{
			UE_LOG(LogAOA, Warning, TEXT("Attempting to initialize Azure Object Anchors after it was already initialized."));
			return;
		}

		// Reset any currently tracked Azure Object Anchors.
		AOAMap.Empty();

		TWeakPtr<FAzureObjectAnchorsPlugin> WeakSelf = SharedThis(this);
		auto RequestAccessOp = OA::ObjectObserver::RequestAccessAsync();
		RequestAccessOp.Completed([WeakSelf](
			WF::IAsyncOperation<OA::ObjectObserverAccessStatus> AsyncOp,
			WF::AsyncStatus Status)
		{
			if (TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin())
			{
				if (Status != WF::AsyncStatus::Completed ||
					AsyncOp.GetResults() != OA::ObjectObserverAccessStatus::Allowed)
				{
					UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors RequestAccessAsync failed with AsyncStatus %d, and ObjectObserverAccessStatus %d"), (int)Status, (int)AsyncOp.GetResults());
					return;
				}

				FGuid AccountID(Self->AzureObjectAnchorConfiguration.AccountID);
				if (!AccountID.IsValid())
				{
					UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors invalid account ID guid."));
					return;
				}

				winrt::hstring AccountKey = winrt::hstring(*Self->AzureObjectAnchorConfiguration.AccountKey);
				winrt::hstring AccountDomain = winrt::hstring(*Self->AzureObjectAnchorConfiguration.AccountDomain);
				OA::AccountInformation AccountInfo(WMRUtility::FGUIDToGuid(AccountID), AccountKey, AccountDomain);

				FScopeLock Lock(&Self->MeshComponentLock);
				Self->ObjectAnchorSession = OA::ObjectAnchorsSession(AccountInfo);
				Self->ObjectAnchorObserver = Self->ObjectAnchorSession.CreateObjectObserver();

				// Set the models the ObjectObserver will look for.  These must have gone through the Object Anchor parser.
				// Models in the game project must be manually included in the package:
				// Project Settings > Packaging > Additional Non-Asset Directories To Copy
				// Or they should be placed in the 3D Objects directory on the device, and the Objects3D capability must be set.
				Self->LoadObjectModelsAsync(winrt::Windows::ApplicationModel::Package::Current().InstalledLocation().Path());
				Self->LoadObjectModelsAsync(winrt::Windows::Storage::KnownFolders::Objects3D().Path());
			}
		});
	}

	void FAzureObjectAnchorsPlugin::StopAzureObjectAnchors()
	{
		FScopeLock Lock(&MeshComponentLock);
		IsAzureObjectAnchorsStartDesired = false;

		if (DetectTask)
		{
			DetectTask.Cancel();
		}

		if (ObjectAnchorObserver)
		{
			ObjectAnchorObserver.Close();
			ObjectAnchorObserver = nullptr;
		}
	}

	void FAzureObjectAnchorsPlugin::ResetObjectSearchAreaAroundHead()
	{
		FScopeLock Lock(&MeshComponentLock);
		// Clear any search areas so queries will look around the current pose.
		SearchAreas.clear();

		// Trigger new search queries
		for (auto& data : AOAMap)
		{
			data.Value.ResetObjectInstance(TrackedMeshHolder);
		}
	}

	void FAzureObjectAnchorsPlugin::ResetObjectSearchAreaAroundPoint(FVector Point, float Radius, bool ClearExistingSearchAreas)
	{
		FScopeLock Lock(&MeshComponentLock);
		if (ClearExistingSearchAreas)
		{
			SearchAreas.clear();
		}

		FVector PointInTrackingSpace = UHeadMountedDisplayFunctionLibrary::GetTrackingToWorldTransform(GWorld).Inverse().TransformPosition(Point);
		DirectX::XMFLOAT3 Origin = WMRUtility::ToMixedRealityVector(PointInTrackingSpace / WorldToMetersScale);

		OA::SpatialGraph::SpatialSphere sphere =
		{
			WFN::float3(Origin.x, Origin.y, Origin.z),
			Radius / WorldToMetersScale
		};

		SearchAreas.push_back(sphere);

		// Trigger new search queries
		for (auto& data : AOAMap)
		{
			data.Value.ResetObjectInstance(TrackedMeshHolder);
		}
	}

	TArray<FARTraceResult> FAzureObjectAnchorsPlugin::OnLineTraceTrackedObjects(const TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe> ARCompositionComponent, const FVector Start, const FVector End, const EARLineTraceChannels TraceChannels)
	{
		TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe> ARSystem = ARCompositionComponent;
		if (ARSystem == nullptr)
		{
			ARSystem = TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe>{
				StaticCastSharedPtr<FXRTrackingSystemBase>(GEngine->XRSystem)->GetARCompositionComponent() };
		}

		TArray<FARTraceResult> Results;

		FScopeLock Lock(&MeshComponentLock);
		TArray<UARMeshGeometry*> Meshes = UARBlueprintLibrary::GetAllGeometriesByClass<UARMeshGeometry>();
		for (auto& data : AOAMap)
		{
			if (data.Value.CollisionInfo == nullptr)
			{
				continue;
			}

			for (UARMeshGeometry* Mesh : Meshes)
			{
				if (Mesh->UniqueId == data.Key)
				{
					FVector HitPoint, HitNormal;
					float HitDistance;
					if (data.Value.CollisionInfo->Collides(Start, End, Mesh->GetLocalToWorldTransform(), HitPoint, HitNormal, HitDistance))
					{
						// Append a hit.  The calling function will then sort by HitDistance.
						Results.Add(FARTraceResult(ARSystem, HitDistance, TraceChannels,
							FTransform(HitNormal.ToOrientationQuat(), HitPoint), Mesh));
					}

					break;
				}
			}
		}

		return Results;
	}

	void FAzureObjectAnchorsPlugin::LoadObjectModelsAsync(winrt::hstring Path)
	{
		TWeakPtr<FAzureObjectAnchorsPlugin> WeakSelf = SharedThis(this);
		auto GetFolderTask = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(Path);
		GetFolderTask.Completed([WeakSelf](WF::IAsyncOperation<winrt::Windows::Storage::StorageFolder> AsyncOp, WF::AsyncStatus Status)
		{
			if (Status != WF::AsyncStatus::Completed)
			{
				UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors GetFolderFromPathAsync failed."));
				return;
			}

			auto GetItemsTask = AsyncOp.GetResults().GetItemsAsync();
			GetItemsTask.Completed([WeakSelf](
				WF::IAsyncOperation<
				WF::Collections::IVectorView<winrt::Windows::Storage::IStorageItem>> AsyncOp, WF::AsyncStatus Status)
			{
				TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin();
				if (Self == nullptr)
				{
					return;
				}

				if (Status != WF::AsyncStatus::Completed)
				{
					UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors GetItemsAsync failed."));
					return;
				}

				for (auto const& item : AsyncOp.GetResults())
				{
					if (item.IsOfType(winrt::Windows::Storage::StorageItemTypes::Folder))
					{
						// Load models in subdirectories.
						Self->LoadObjectModelsAsync(item.as<winrt::Windows::Storage::StorageFolder>().Path());
						continue;
					}

					const auto file = item.as<winrt::Windows::Storage::StorageFile>();
					// Object Anchor service requires the input models to have gone through its parser before being used by the observer.
					if (file.FileType() != L".ou")
					{
						continue;
					}

					auto ReadBufferTask = winrt::Windows::Storage::FileIO::ReadBufferAsync(file);
					ReadBufferTask.Completed([WeakSelf](WF::IAsyncOperation<winrt::Windows::Storage::Streams::IBuffer> AsyncOp, WF::AsyncStatus Status)
					{
						TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin();
						if (Self == nullptr)
						{
							return;
						}

						if (Status != WF::AsyncStatus::Completed)
						{
							UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors ReadBufferAsync failed."));
							return;
						}

						winrt::Windows::Storage::Streams::IBuffer Buffer = AsyncOp.GetResults();

						FScopeLock Lock(&Self->MeshComponentLock);
						if (Self->ObjectAnchorObserver == nullptr)
						{
							return;
						}
						auto LoadObjectTask = Self->ObjectAnchorObserver.LoadObjectModelAsync(
							winrt::array_view(Buffer.data(), Buffer.Length()));
						LoadObjectTask.Completed([WeakSelf, Buffer](WF::IAsyncOperation<OA::ObjectModel> AsyncOp, WF::AsyncStatus Status)
						{
							TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin();
							if (Self == nullptr)
							{
								return;
							}

							if (Status != WF::AsyncStatus::Completed)
							{
								UE_LOG(LogAOA, Warning, TEXT("Azure Object Anchors LoadObjectModelAsync failed."));
								return;
							}

							OA::ObjectModel Model = AsyncOp.GetResults();
							if (!Self->AOAMap.Contains(WMRUtility::GUIDToFGuid(Model.Id())))
							{
								AsyncTask(ENamedThreads::GameThread, [WeakSelf, Model]()
								{
									TSharedPtr<FAzureObjectAnchorsPlugin> Self = WeakSelf.Pin();
									if (Self == nullptr)
									{
										return;
									}

									FScopeLock Lock(&Self->MeshComponentLock);
									AOAContext Context;
									Context.ObjectModel = Model;

									// Load the mesh data to set the collision mesh.
									std::vector<WFN::float3> SrcVertices(Model.VertexCount());
									std::vector<uint32_t> SrcIndices(Model.TriangleIndexCount());

									Model.GetVertexPositions(SrcVertices);
									Model.GetTriangleIndices(SrcIndices);

									TArray<FVector3f> Vertices;
									TArray<MRMESH_INDEX_TYPE> Indices;

									Vertices.AddUninitialized(Model.VertexCount());
									Indices.AddUninitialized(Model.TriangleIndexCount());

									// MRMesh uses uint16 for indices so we cannot directly memcpy
									for (size_t i = 0; i < Model.TriangleIndexCount(); i++)
									{
										Indices[i] = (MRMESH_INDEX_TYPE)SrcIndices[i];
									}

									for (size_t i = 0; i < Model.VertexCount(); i++)
									{
										Vertices[i] = MicrosoftOpenXR::WMRUtility::FromFloat3ToFVector3f(SrcVertices[i], Self->WorldToMetersScale);
									}

									Context.CollisionInfo = new TrackedGeometryCollision(Vertices, Indices);

									// Update the MRMesh data with the chosen render mode.
									Self->TrackedMeshHolder->StartMeshUpdates();
									FOpenXRMeshUpdate* MeshUpdate = Self->TrackedMeshHolder->AllocateMeshUpdate(WMRUtility::GUIDToFGuid(Model.Id()));

									if (Self->AzureObjectAnchorConfiguration.ObjectRenderMode == EObjectRenderMode::Mesh)
									{
										// Use the full mesh data in the MRMesh
										Context.Indices = Indices;
										Context.Vertices = Vertices;
									}
									else if (Self->AzureObjectAnchorConfiguration.ObjectRenderMode == EObjectRenderMode::BoundingBox)
									{
										// Create an indexed primitive from the bounding box rather than using all of the model mesh data.
										// This can help performance if the object model has too many polys.

										FVector3f Center = WMRUtility::FromFloat3ToFVector3f(Model.BoundingBox().Center, Self->WorldToMetersScale);
										FVector3f HalfExtents = WMRUtility::FromFloat3ToFVector3f(Model.BoundingBox().Extents / 2.0f, Self->WorldToMetersScale);

										TrackedGeometryCollision::CreateMeshDataForBoundingBox(Center, HalfExtents, Context.Vertices, Context.Indices);
									}

									MeshUpdate->Indices = Context.Indices;
									MeshUpdate->Vertices = Context.Vertices;

									MeshUpdate->Type = EARObjectClassification::SceneObject;
									MeshUpdate->LocalToTrackingTransform = FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
									MeshUpdate->SpatialMeshUsageFlags =
										(EARSpatialMeshUsageFlags)((int32)EARSpatialMeshUsageFlags::Visible |
											(int32)EARSpatialMeshUsageFlags::Collision);

									Self->TrackedMeshHolder->EndMeshUpdates();

									Self->AOAMap.Add(WMRUtility::GUIDToFGuid(Model.Id()), Context);
								});
							}
						});
					});
				}
			});
		});
	}

} // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
