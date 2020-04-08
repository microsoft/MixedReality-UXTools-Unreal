// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Manipulation/UxtManipulationMoveLogic.h"
#include "Utils/UxtFunctionLibrary.h"
#include "CoreMinimal.h"

void UxtManipulationMoveLogic::Setup(const FTransform& PointerCentroidPose, const FVector& GrabCentroid, const FTransform& ObjectTransform, const FVector& HeadPosition)
{
	PointerRefDistance = GetDistanceToBody(PointerCentroidPose.GetLocation(), HeadPosition);
	PointerPosIndependenOfHead = PointerRefDistance != 0;

	FQuat WorldToPointerRotation = PointerCentroidPose.GetRotation().Inverse();
	PointerLocalGrabPoint = WorldToPointerRotation * (GrabCentroid - PointerCentroidPose.GetLocation());

	ObjectLocalGrabPoint = ObjectTransform.GetRotation().Inverse() * (GrabCentroid - ObjectTransform.GetLocation());
	ObjectLocalGrabPoint = ObjectLocalGrabPoint / ObjectTransform.GetScale3D();

	GrabToObject = ObjectTransform.GetLocation() - GrabCentroid;
}

FVector UxtManipulationMoveLogic::Update(const FTransform& PointerCentroidPose, const FQuat& ObjectRotation, const FVector& ObjectScale, bool UsePointerRotation, const FVector& HeadPosition)
{
	float DistanceRatio = 1.0f;

	if (PointerPosIndependenOfHead)
	{
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
		return PointerCentroidPose.GetLocation() + (PointerCentroidPose.GetRotation() * PointerLocalGrabPoint + GrabToObject) * DistanceRatio;
	}
}

float UxtManipulationMoveLogic::GetDistanceToBody(const FVector& PointerCentroidPosition, const FVector& HeadPosition)
{
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


