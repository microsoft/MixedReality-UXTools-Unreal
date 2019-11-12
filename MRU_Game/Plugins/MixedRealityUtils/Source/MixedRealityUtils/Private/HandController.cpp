// Fill out your copyright notice in the Description page of Project Settings.


#include "HandController.h"
#include "MixedRealityHand.h"

void IHandController::UpdateHand(UObject *controller, AMixedRealityHand *hand)
{
	FTransform rootTransform = IHandController::Execute_GetRootTransform(controller, hand->Handedness);
	hand->SetActorTransform(rootTransform);

	FVector touchLocation = IHandController::Execute_GetTouchLocation(controller, hand->Handedness);
	hand->SetTouchPointerWorldLocation(touchLocation);
}
