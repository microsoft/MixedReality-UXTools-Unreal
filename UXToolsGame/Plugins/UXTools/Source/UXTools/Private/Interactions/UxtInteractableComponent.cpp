// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtInteractableComponent.h"
#include "Input/UxtTouchPointer.h"
#include "Input/UxtFarPointerComponent.h"


UUxtInteractableComponent::UUxtInteractableComponent()
{
}

void UUxtInteractableComponent::HoverStarted_Implementation(UUxtTouchPointer* Pointer)
{
	NumPointersFocusing++;
    ActiveTouchPointers.Add(TWeakObjectPtr<UUxtTouchPointer>(Pointer));
	const bool bWasHovered = NumPointersFocusing > 1;
	OnHoverStarted.Broadcast(this, Pointer, bWasHovered);
}

void UUxtInteractableComponent::HoverEnded_Implementation(UUxtTouchPointer* Pointer)
{
	NumPointersFocusing--;
    ActiveTouchPointers.Remove(TWeakObjectPtr<UUxtTouchPointer>(Pointer));
	const bool bIsHovered = NumPointersFocusing > 0;
	OnHoverEnded.Broadcast(this, Pointer, bIsHovered);
}

void UUxtInteractableComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent)
{
	NumPointersFocusing++;
	ActiveFarPointers.Add(TWeakObjectPtr<UUxtFarPointerComponent>(Pointer));
	const bool bWasHovered = NumPointersFocusing > 1;
	OnHoverStarted.Broadcast(this, Pointer, bWasHovered);
}

void UUxtInteractableComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent)
{
	NumPointersFocusing--;
	ActiveFarPointers.Remove(TWeakObjectPtr<UUxtFarPointerComponent>(Pointer));
	const bool bIsHovered = NumPointersFocusing > 0;
	OnHoverEnded.Broadcast(this, Pointer, bIsHovered);
}

bool UUxtInteractableComponent::GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface)
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

TArray<UUxtTouchPointer*> UUxtInteractableComponent::GetActiveTouchPointers() const
{
	TArray<UUxtTouchPointer*> result;
	result.Reserve(ActiveTouchPointers.Num());
	for (const TWeakObjectPtr<UUxtTouchPointer> &wPtr : ActiveTouchPointers)
	{
		if (UUxtTouchPointer *ptr = wPtr.Get())
		{
			result.Add(ptr);
		}
	}
	return result;
}
