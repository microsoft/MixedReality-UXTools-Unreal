// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/UxtGrabTargetComponent.h"

/**
 * Implements a scale logic that will scale an object based on the ratio of the
 * distance between hands:
 * object_scale = start_object_scale * curr_hand_dist / start_hand_dist
 *
 * Usage:
 * When a manipulation starts, call Setup.
 * Call Update with currently available grab pointers to get a new scale for the object.
 */
class UxtTwoHandManipulationScaleLogic
{
public:
	typedef const TArray<FUxtGrabPointerData>& GrabPointers;
	/** Sets up scale logic by storing initial object scale and hand distance */
	void Setup(GrabPointers PointerData, const FVector& ObjectScale);

	/** Updates the scale based on the current grab pointer locations. Returns the new object scale. */
	FVector Update(GrabPointers PointerData) const;

private:
	FVector StartObjectScale;
	float StartHandDistanceMeters;
};
