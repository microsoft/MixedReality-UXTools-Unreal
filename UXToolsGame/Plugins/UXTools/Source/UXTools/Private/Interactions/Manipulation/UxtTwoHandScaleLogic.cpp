#include "UxtTwoHandScaleLogic.h"

namespace
{
	float GetMinDistanceBetweenHands(UxtTwoHandManipulationScaleLogic::GrabPointers PointerData)
	{
		float result = TNumericLimits<float>::Max();
		for (int i = 0; i < PointerData.Num(); i++)
		{
			for (int j = i + 1; j < PointerData.Num(); j++)
			{
				FVector handLocationFirst = UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[i]);
				FVector handLocationSecond = UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[j]);
				float distance = FVector::Dist(handLocationFirst, handLocationSecond);
				if (distance < result)
				{
					result = distance;
				}
			}
		}
		return result;
	}
}

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
