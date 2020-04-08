// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include <vector>
#include "Interactions/UxtGrabTargetComponent.h"


class UxtTwoHandManipulationRotateLogic
{
public:
	typedef const TArray<FUxtGrabPointerData>& GrabPointers;
	void Setup(GrabPointers PointerData, const FQuat& HostRotation);

	FQuat Update(GrabPointers PointerData) const;

private:

	static FVector GetHandleBarDirection(GrabPointers PointerData);

	FVector StartHandleBar;
	FQuat StartRotation;
};