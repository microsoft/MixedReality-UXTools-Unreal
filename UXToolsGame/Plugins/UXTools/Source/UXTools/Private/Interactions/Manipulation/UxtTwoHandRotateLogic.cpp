#include "Interactions/Manipulation/UxtTwoHandRotateLogic.h"
#include "Interactions/UxtGrabTargetComponent.h"

void UxtTwoHandManipulationRotateLogic::Setup(GrabPointers PointerData, const FQuat& HostRotation)
{
	StartHandleBar = GetHandleBarDirection(PointerData);
	StartRotation = HostRotation;
}

FQuat UxtTwoHandManipulationRotateLogic::Update(GrabPointers PointerData, const FQuat& CurrentRotation) const
{
	FQuat Rot = FQuat::FindBetweenVectors(StartHandleBar, GetHandleBarDirection(PointerData));

	//FVector axis = FVector::CrossProduct(StartHandleBar, GetHandleBarDirection(PointerData));
	//float angle = FVector::Angle(StartHandleBar, GetHandleBarDirection(PointerData));
	//return FQuat::AngleAxis(angle, axis.normalized);

	//Rot.Normalize();
	Rot = Rot * StartRotation;
	Rot.Normalize();
	return Rot;
}

FVector UxtTwoHandManipulationRotateLogic::GetHandleBarDirection(GrabPointers PointerData)
{
	if (PointerData.Num() > 1)
	{
		return UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[1]) - UUxtGrabPointerDataFunctionLibrary::GetPointerLocation(PointerData[0]);
	}

	return FVector::ZeroVector;
}

