// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

class UxtManipulationMoveLogic
{
public:
	void Setup(const FTransform& PointerCentroidPose, const FVector& GrabCentroid, const FTransform& ObjectTransform, const FVector& HeadPosition);

	FVector Update(const FTransform& PointerCentroidPose, const FQuat& ObjectRotation, const FVector& ObjectScale, bool UsePointerRotation, const FVector& HeadPosition);



private:

	float GetDistanceToBody(const FVector& PointerCentroidPosition, const FVector& HeadPosition);

	float PointerRefDistance;
	bool PointerPosIndependenOfHead = true;
	FVector PointerLocalGrabPoint;
	FVector ObjectLocalGrabPoint;
	FVector GrabToObject;
};