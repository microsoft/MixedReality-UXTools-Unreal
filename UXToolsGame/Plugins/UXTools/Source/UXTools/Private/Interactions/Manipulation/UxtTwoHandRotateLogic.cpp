// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTwoHandRotateLogic.h"

namespace
{
	FVector GetHandleBarDirection(UxtTwoHandManipulationRotateLogic::GrabPointers PointerData)
	{
		if (PointerData.Num() > 1)
		{
			return UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[1]) -
				   UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[0]);
		}

		return FVector::ZeroVector;
	}
} // namespace

void UxtTwoHandManipulationRotateLogic::Setup(GrabPointers PointerData, const FQuat& HostRotation)
{
	StartHandleBar = GetHandleBarDirection(PointerData);
	StartRotation = HostRotation;
}

FQuat UxtTwoHandManipulationRotateLogic::Update(GrabPointers PointerData) const
{
	FVector UpdatedHandleBar = GetHandleBarDirection(PointerData);
	FQuat Rot = FQuat::FindBetween(StartHandleBar, UpdatedHandleBar);
	Rot.Normalize();
	return Rot * StartRotation;
}
