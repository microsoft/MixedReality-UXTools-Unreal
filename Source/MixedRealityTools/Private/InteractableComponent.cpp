// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableComponent.h"


UInteractableComponent::UInteractableComponent()
{
}

void UInteractableComponent::TouchStarted_Implementation(USceneComponent* pointer)
{
    ActivePointers.Add(TWeakObjectPtr<USceneComponent>(pointer));
}

void UInteractableComponent::TouchEnded_Implementation(USceneComponent* pointer)
{
    ActivePointers.Remove(TWeakObjectPtr<USceneComponent>(pointer));
}
