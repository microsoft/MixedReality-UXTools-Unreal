// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableComponent.h"
#include "TouchPointer.h"


UInteractableComponent::UInteractableComponent()
{
}

void UInteractableComponent::TouchStarted_Implementation(UTouchPointer* Pointer)
{
    ActivePointers.Add(TWeakObjectPtr<UTouchPointer>(Pointer));
}

void UInteractableComponent::TouchEnded_Implementation(UTouchPointer* Pointer)
{
    ActivePointers.Remove(TWeakObjectPtr<UTouchPointer>(Pointer));
}

bool UInteractableComponent::GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface)
{
	return false;
}

TArray<UTouchPointer*> UInteractableComponent::GetActivePointers() const
{
	TArray<UTouchPointer*> result;
	result.Reserve(ActivePointers.Num());
	for (const TWeakObjectPtr<UTouchPointer> &wPtr : ActivePointers)
	{
		if (UTouchPointer *ptr = wPtr.Get())
		{
			result.Add(ptr);
		}
	}
	return result;
}
