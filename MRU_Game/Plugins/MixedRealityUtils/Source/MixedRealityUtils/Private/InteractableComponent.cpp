// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableComponent.h"
#include "TouchPointer.h"


UInteractableComponent::UInteractableComponent()
{
}

void UInteractableComponent::HoverStarted_Implementation(UTouchPointer* Pointer)
{
    ActivePointers.Add(TWeakObjectPtr<UTouchPointer>(Pointer));
}

void UInteractableComponent::HoverEnded_Implementation(UTouchPointer* Pointer)
{
    ActivePointers.Remove(TWeakObjectPtr<UTouchPointer>(Pointer));
}

bool UInteractableComponent::GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface)
{
	OutPointOnSurface = Point;
	float ClosestPointDistanceSqr = -1.f;

	TInlineComponentArray<UPrimitiveComponent*> Components;
	GetOwner()->GetComponents(Components);

	// This is exactly what happens inside AActor::ActorGetDistanceToCollision except we're skipping the collision response check
	// because the pointer will usually be configured to overlap with this actor's primitives, not block.
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UPrimitiveComponent* Primitive = Components[ComponentIndex];
		if (Primitive->IsRegistered() && Primitive->IsCollisionEnabled())
		{
			FVector ClosestPoint;
			float DistanceSqr = -1.f;

			if (!Primitive->GetSquaredDistanceToCollision(Point, DistanceSqr, ClosestPoint))
			{
				// Invalid result, impossible to be better than ClosestPointDistance
				continue;
			}

			if ((ClosestPointDistanceSqr < 0.f) || (DistanceSqr < ClosestPointDistanceSqr))
			{
				ClosestPointDistanceSqr = DistanceSqr;
				OutPointOnSurface = ClosestPoint;

				// If we're inside collision, we're not going to find anything better, so abort search we've got our best find.
				if (DistanceSqr <= KINDA_SMALL_NUMBER)
				{
					break;
				}
			}
		}
	}

	return ClosestPointDistanceSqr >= 0.f;
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
