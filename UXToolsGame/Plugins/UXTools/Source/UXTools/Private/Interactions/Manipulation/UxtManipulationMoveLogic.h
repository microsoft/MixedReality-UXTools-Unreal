// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

/**
 * Implements a move logic that will move an object based on the initial position of
 * the grab point relative to the pointer and relative to the object, and subsequent
 * changes to the pointer and the object's rotation
 *
 * Usage:
 * When a manipulation starts, call Setup.
 * Call Update for querying a new position for the object.
 */
class UxtManipulationMoveLogic
{
public:
	/** Setup move logic by caching initial input values */
	void Setup(
		const FTransform& PointerCentroidPose, const FVector& GrabCentroid, const FTransform& ObjectTransform, const FVector& HeadPosition);

	/** Provide updated input and head/camera position to retrieve new object position*/
	FVector Update(
		const FTransform& PointerCentroidPose, const FQuat& ObjectRotation, const FVector& ObjectScale, bool UsePointerRotation,
		const FVector& HeadPosition) const;

private:
	float PointerRefDistance;
	bool PointerPosIndependenOfHead = true;
	FVector PointerLocalGrabPoint;
	FVector ObjectLocalGrabPoint;
	FVector GrabToObject;
};
