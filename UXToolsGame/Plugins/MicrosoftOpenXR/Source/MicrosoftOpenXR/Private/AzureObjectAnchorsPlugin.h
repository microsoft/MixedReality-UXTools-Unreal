// Copyright (c) 2022 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "OpenXRCommon.h"
#include "OpenXRCore.h"
#include "MicrosoftOpenXR.h"

#include <winrt/Windows.Perception.Spatial.Preview.h>
#include <winrt/Microsoft.Azure.ObjectAnchors.h>
#include <winrt/Windows.Foundation.Numerics.h>

#include "Async/Async.h"
#include "Engine/Engine.h"

#include "WindowsMixedRealityInteropUtility.h"

#include "IOpenXRARTrackedGeometryHolder.h"
#include "IOpenXRARModule.h"

#include "TrackedGeometryCollision.h"
#include "ARBlueprintLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"

namespace OA = winrt::Microsoft::Azure::ObjectAnchors;

namespace MicrosoftOpenXR
{
	class FAzureObjectAnchorsPlugin : 
		public IOpenXRExtensionPlugin, 
		public IOpenXRCustomCaptureSupport, 
		public TSharedFromThis<FAzureObjectAnchorsPlugin>
	{
	public:
		void Register();
		void Unregister();

		~FAzureObjectAnchorsPlugin();

		void OnStartARSession(class UARSessionConfig* SessionConfig) override;

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		virtual const void* OnBeginSession(XrSession InSession, const void* InNext) override;
		void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		bool OnToggleARCapture(const bool bOnOff) override;
		void InitAzureObjectAnchors(FAzureObjectAnchorSessionConfiguration AOAConfiguration);
		void ResetObjectSearchAreaAroundHead();
		void ResetObjectSearchAreaAroundPoint(FVector Point, float Radius, bool ClearExistingSearchAreas);

		TArray<FARTraceResult> OnLineTraceTrackedObjects(const TSharedPtr<FARSupportInterface, ESPMode::ThreadSafe> ARCompositionComponent, const FVector Start, const FVector End, const EARLineTraceChannels TraceChannels) override;

	private:
		XrSession Session;

		PFN_xrCreateSpatialGraphNodeSpaceMSFT xrCreateSpatialGraphNodeSpaceMSFT;
		PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
		PFN_xrTryCreateSpatialGraphStaticNodeBindingMSFT xrTryCreateSpatialGraphStaticNodeBindingMSFT;
		PFN_xrGetSpatialGraphNodeBindingPropertiesMSFT xrGetSpatialGraphNodeBindingPropertiesMSFT;

		bool IsAzureObjectAnchorsStartDesired = false;

		bool CanUseSpatialGraphExtension = false;
		bool ARSessionStarted = false;
		bool FrameOfReferenceLocated = false;
		bool IsCurrentlyDetecting = false;

		winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<OA::ObjectInstance>> DetectTask = nullptr;

		FAzureObjectAnchorSessionConfiguration AzureObjectAnchorConfiguration;
		OA::SpatialGraph::SpatialSphere SearchSphere;
		std::vector<OA::ObjectQuery> Queries;

		IOpenXRARTrackedMeshHolder* TrackedMeshHolder = nullptr;
		float WorldToMetersScale = 100.0f;

		OA::ObjectAnchorsSession ObjectAnchorSession = nullptr;
		OA::ObjectObserver ObjectAnchorObserver = nullptr;

		OA::SpatialGraph::SpatialGraphCoordinateSystem OriginCoordinateSystem;
		XrSpace OriginCoordinateSystemTrackingSpace = XR_NULL_HANDLE;

		struct AOAContext
		{
			winrt::guid SpatialGraphNodeId;
			XrSpace Space = XR_NULL_HANDLE;

			OA::ObjectModel ObjectModel = nullptr;
			OA::ObjectInstance ObjectInstance = nullptr;
			winrt::event_token ObjectInstanceChangedEventToken;

			bool HasChanged = false;
			OA::ObjectInstanceState LastKnownState;

			TArray<FVector3f> Vertices;
			TArray<MRMESH_INDEX_TYPE> Indices;

			TrackedGeometryCollision* CollisionInfo = nullptr;

		public:
			void ResetObjectInstance(IOpenXRARTrackedMeshHolder* TrackedMeshHolder)
			{
				if (ObjectInstance != nullptr)
				{
					if (ObjectInstanceChangedEventToken.value != 0)
					{
						ObjectInstance.Changed(ObjectInstanceChangedEventToken);
					}

					ObjectInstance.Close();

					TrackedMeshHolder->StartMeshUpdates();
					TrackedMeshHolder->RemoveMesh(WMRUtility::GUIDToFGuid(ObjectModel.Id()));
					TrackedMeshHolder->EndMeshUpdates();
				}

				ObjectInstanceChangedEventToken.value = 0;

				if (Space != XR_NULL_HANDLE)
				{
					xrDestroySpace(Space);
					Space = XR_NULL_HANDLE;
				}

				SpatialGraphNodeId = winrt::guid();
				HasChanged = false;
			}

			bool IsCurrentlyBeingTracked()
			{
				return ObjectInstanceChangedEventToken.value != 0;
			}

			// This can happen if the device loses tracking:
			// The underlying object instance can be closed without triggering a changed event.
			// Reset the object instance to find a new one.
			void ResetStaleObjectInstance(IOpenXRARTrackedMeshHolder* TrackedMeshHolder)
			{
				if (IsCurrentlyBeingTracked() &&
					(ObjectInstance == nullptr || 
					ObjectInstance.TryGetCurrentState() == nullptr))
				{
					ResetObjectInstance(TrackedMeshHolder);
				}
			}
		};

		FCriticalSection MeshComponentLock;
		TMap<FGuid, AOAContext> AOAMap;

		std::vector<OA::SpatialGraph::SpatialSphere> SearchAreas;

		void StopAzureObjectAnchors();
		void LoadObjectModelsAsync(winrt::hstring Path);
		void UpdateSpatialGraphInfoForObjectInstance(OA::ObjectInstance Object);
	};
}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS