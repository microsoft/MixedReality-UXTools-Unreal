// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/UxtGrabTargetComponent.h"

/**
 * Implements common logic for rotating holograms using a handlebar metaphor.
 *
 * Each frame, object_rotation_delta = rotation_delta(current_hands_vector, previous_hands_vector)
 * where hands_vector is the vector between two hand/controller positions.
 *
 * Usage:
 * When a manipulation starts, call Setup.
 * Call Update with currently available grab pointers to get a new rotation for the object.
 */
class UxtTwoHandManipulationRotateLogic
{
public:
	typedef const TArray<FUxtGrabPointerData>& GrabPointers;
	/** Sets up rotation logic by storing initial handle bar and rotation value */
	void Setup(GrabPointers PointerData, const FQuat& HostRotation);

	/** Updates the rotation based on the current grab pointer locations */
	FQuat Update(GrabPointers PointerData) const;

private:
	FVector StartHandleBar;
	FQuat StartRotation;
};
