// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTwoHandScaleLogic.h"

namespace
{
	float GetMinDistanceBetweenHands(UxtTwoHandManipulationScaleLogic::GrabPointers PointerData)
	{
		float Result = TNumericLimits<float>::Max();
		for (int i = 0; i < PointerData.Num(); i++)
		{
			for (int j = i + 1; j < PointerData.Num(); j++)
			{
				FVector HandLocationFirst = UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[i]);
				FVector HandLocationSecond = UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[j]);
				float Distance = FVector::Dist(HandLocationFirst, HandLocationSecond);
				if (Distance < Result)
				{
					Result = Distance;
				}
			}
		}
		return Result;
	}
} // namespace

void UxtTwoHandManipulationScaleLogic::Setup(GrabPointers PointerData, const FVector& ObjectScale)
{
	StartHandDistanceMeters = GetMinDistanceBetweenHands(PointerData);
	StartObjectScale = ObjectScale;
}

FVector UxtTwoHandManipulationScaleLogic::Update(GrabPointers PointerData) const
{
	float ratioMultiplier = GetMinDistanceBetweenHands(PointerData) / StartHandDistanceMeters;
	return StartObjectScale * ratioMultiplier;
}
