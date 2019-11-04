// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableSceneComponent.h"


UInteractableSceneComponent::UInteractableSceneComponent()
{
}

void UInteractableSceneComponent::TouchStarted_Implementation(USceneComponent* pointer)
{
    ActivePointers.Add(TWeakObjectPtr<USceneComponent>(pointer));
}

void UInteractableSceneComponent::TouchEnded_Implementation(USceneComponent* pointer)
{
    ActivePointers.Remove(TWeakObjectPtr<USceneComponent>(pointer));
}

bool UInteractableSceneComponent::GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface)
{
	return false;
}
