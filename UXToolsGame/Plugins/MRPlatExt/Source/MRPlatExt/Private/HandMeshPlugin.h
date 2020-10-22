#pragma once

#include "OpenXRCommon.h"
#include "ARTypes.h"
#include "MRPlatExt.h"
#include "IHandTracker.h"

#include <vector>

class IOpenXRARTrackedMeshHolder;

namespace MRPlatExt
{
	class FHandMeshPlugin : public IOpenXRExtensionPlugin, public IHandTracker
	{
	public:
		static const int HandCount = 2;
		enum Hand {Left = 0, Right = 1};

		struct FHandState : public FNoncopyable
		{
			FHandState();

			XrHandTrackerEXT HandTracker{};
			XrSpace Space{};

			std::vector<uint32_t> Indices;
			std::vector<XrHandMeshVertexMSFT> Vertices;

			size_t VerticesCount = 0;
			size_t IndicesCount = 0;

			size_t VerticesMaxAmount = 0;
			size_t IndicesMaxAmount = 0;

			FGuid Guid;

			EARTrackingState TrackingState = EARTrackingState::Unknown;
			FTransform LocalToTrackingTransform;
		};

		void Register();
		void Unregister();

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		const void* OnBeginSession(XrSession InSession, const void* InNext) override;
		void PostSyncActions(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		bool Turn(EHandMeshStatus Mode);
	private:
		FHandState HandStates[HandCount];

		EHandMeshStatus HandMeshStatus = EHandMeshStatus::NotInitialised;

		PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT = nullptr;

		PFN_xrCreateHandMeshSpaceMSFT xrCreateHandMeshSpaceMSFT = nullptr;
		PFN_xrUpdateHandMeshMSFT xrUpdateHandMeshMSFT = nullptr;

		class IXRTrackingSystem* XRTrackingSystem = nullptr;
		IOpenXRARTrackedMeshHolder* TrackedMeshHolder = nullptr;

		// Inherited via IHandTracker
		virtual FName GetHandTrackerDeviceTypeName() const override
		{
			return FName(TEXT("MRPlatExtHandTracking"));
		}

		virtual bool IsHandTrackingStateValid() const override;

		virtual bool GetKeypointState(EControllerHand Hand, EHandKeypoint Keypoint, FTransform& OutTransform, float& OutRadius) const override
		{
			return false;
		}

		virtual bool GetAllKeypointStates(EControllerHand Hand, TArray<struct FVector>& OutPositions, TArray<struct FQuat>& OutRotations, TArray<float>& OutRadii) const override
		{
			return false;
		}

		virtual bool HasHandMeshData() const override;

		virtual bool GetHandMeshData(EControllerHand Hand, TArray<struct FVector>& OutVertices, TArray<struct FVector>& OutNormals, TArray<int32>& OutIndices, FTransform& OutHandMeshTransform) const override;
	};
}	 // namespace MRPlatExt