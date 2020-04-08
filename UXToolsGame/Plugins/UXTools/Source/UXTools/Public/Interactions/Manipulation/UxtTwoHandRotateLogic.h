// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"
#include <vector>
#include "../UxtGrabTargetComponent.h"

//struct FUxtGrabPointerData;

class UxtTwoHandManipulationRotateLogic
{
public:
	typedef const TArray<FUxtGrabPointerData>& GrabPointers;
	void Setup(GrabPointers PointerData, const FQuat& HostRotation);

	FQuat Update(GrabPointers PointerData, const FQuat& CurrentRotation) const;

private:

	static FVector GetHandleBarDirection(GrabPointers PointerData);

	FVector StartHandleBar;
	FQuat StartRotation;
};