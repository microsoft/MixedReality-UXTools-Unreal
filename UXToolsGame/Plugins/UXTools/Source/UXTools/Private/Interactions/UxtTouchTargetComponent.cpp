// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtTouchTargetComponent.h"
#include "Interactions/UxtInteractionUtils.h"

UUxtTouchTargetComponent::UUxtTouchTargetComponent()
{
}

void UUxtTouchTargetComponent::OnEnterTouchFocus_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	FocusedPointers.Add(Pointer, Data);
	const bool bWasFocused = FocusedPointers.Num() != 1;
	OnBeginFocus.Broadcast(this, Pointer, Data, bWasFocused);
}

void UUxtTouchTargetComponent::OnUpdateTouchFocus_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	FocusedPointers.FindChecked(Pointer) = Data;
	OnUpdateFocus.Broadcast(this, Pointer, Data);
}

void UUxtTouchTargetComponent::OnExitTouchFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	FocusedPointers.Remove(Pointer);
	const bool bIsFocused = FocusedPointers.Num() > 0;
	OnEndFocus.Broadcast(this, Pointer, bIsFocused);
}

bool UUxtTouchTargetComponent::GetClosestTouchPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const
{
	float DistanceSqr;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutPointOnSurface, DistanceSqr);
}

const TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData>& UUxtTouchTargetComponent::GetFocusedPointers() const
{
	return FocusedPointers;
}
