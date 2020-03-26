// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtTouchTargetComponent.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Input/UxtNearPointerComponent.h"

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

void UUxtTouchTargetComponent::OnBeginTouch_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	TouchPointers.Add(Pointer, Data);

	// Lock the touching pointer so we remain the focused target as it moves.
	Pointer->SetFocusLocked(true);

	OnBeginTouch.Broadcast(this, Pointer, Data);
}

void UUxtTouchTargetComponent::OnUpdateTouch_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	// Update the copy of the pointer data in the grab pointer array
	TouchPointers.FindChecked(Pointer) = Data;
	OnUpdateTouch.Broadcast(this, Pointer, Data);
}

void UUxtTouchTargetComponent::OnEndTouch_Implementation(UUxtNearPointerComponent* Pointer)
{
	TouchPointers.Remove(Pointer);

	// Unlock the pointer focus so that another target can be selected.
	Pointer->SetFocusLocked(false);

	OnEndTouch.Broadcast(this, Pointer);
}

const TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData>& UUxtTouchTargetComponent::GetFocusedPointers() const
{
	return FocusedPointers;
}

const TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData>& UUxtTouchTargetComponent::GetTouchPointers() const
{
	return TouchPointers;
}
