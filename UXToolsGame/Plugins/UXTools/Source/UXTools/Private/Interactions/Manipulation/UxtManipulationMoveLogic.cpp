// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Manipulation/UxtManipulationMoveLogic.h"

#include "CoreMinimal.h"

#include "Utils/UxtFunctionLibrary.h"

namespace
{
	float GetDistanceToBody(const FVector& PointerCentroidPosition, const FVector& HeadPosition)
	{
		// The body is treated as a ray, parallel to the y-axis, where the start is head position.
		// This means that moving your hand down such that is the same distance from the body will
		// not cause the manipulated object to move further away from your hand. However, when you
		// move your hand upward, away from your head, the manipulated object will be pushed away.
		if (PointerCentroidPosition.Z > HeadPosition.Z)
		{
			return FVector::Dist(PointerCentroidPosition, HeadPosition);
		}
		else
		{
			FVector2D HeadPosXZ(HeadPosition.X, HeadPosition.Y);
			FVector2D PointerPosXZ(PointerCentroidPosition.X, PointerCentroidPosition.Y);

			return FVector2D::Distance(PointerPosXZ, HeadPosXZ);
		}
	}
} // namespace

void UxtManipulationMoveLogic::Setup(
	const FTransform& PointerCentroidPose, const FVector& GrabCentroid, const FTransform& ObjectTransform, const FVector& HeadPosition)
{
	PointerRefDistance = GetDistanceToBody(PointerCentroidPose.GetLocation(), HeadPosition);
	PointerPosIndependenOfHead = PointerRefDistance != 0;

	FQuat WorldToPointerRotation = PointerCentroidPose.GetRotation().Inverse();
	PointerLocalGrabPoint = WorldToPointerRotation * (GrabCentroid - PointerCentroidPose.GetLocation());

	ObjectLocalGrabPoint = ObjectTransform.GetRotation().Inverse() * (GrabCentroid - ObjectTransform.GetLocation());
	ObjectLocalGrabPoint = ObjectLocalGrabPoint / ObjectTransform.GetScale3D();

	GrabToObject = ObjectTransform.GetLocation() - GrabCentroid;
}

FVector UxtManipulationMoveLogic::Update(
	const FTransform& PointerCentroidPose, const FQuat& ObjectRotation, const FVector& ObjectScale, bool UsePointerRotation,
	const FVector& HeadPosition) const
{
	float DistanceRatio = 1.0f;

	if (PointerPosIndependenOfHead)
	{
		// Compute how far away the object should be based on the ratio of the current to original hand distance
		float CurrentHandDistance = GetDistanceToBody(PointerCentroidPose.GetLocation(), HeadPosition);
		DistanceRatio = CurrentHandDistance / PointerRefDistance;
	}

	if (UsePointerRotation)
	{
		FVector ScaledGrabToObject = ObjectLocalGrabPoint * ObjectScale;
		FVector AdjustedPointerToGrab = PointerLocalGrabPoint * DistanceRatio;
		AdjustedPointerToGrab = PointerCentroidPose.GetRotation() * AdjustedPointerToGrab;

		return AdjustedPointerToGrab - ObjectRotation * ScaledGrabToObject + PointerCentroidPose.GetLocation();
	}
	else
	{
		return PointerCentroidPose.GetLocation() +
			   (PointerCentroidPose.GetRotation() * PointerLocalGrabPoint + GrabToObject) * DistanceRatio;
	}
}
