// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtTouchTargetComponent.h"
#include "Interactions/UxtInteractionUtils.h"

UUxtTouchTargetComponent::UUxtTouchTargetComponent()
{
}

void UUxtTouchTargetComponent::OnEnterTouchFocus_Implementation(int32 PointerId, const FUxtPointerInteractionData& Data)
{
	FocusedPointers.Add(PointerId, Data);
	const bool bWasFocused = FocusedPointers.Num() != 1;
	OnBeginFocus.Broadcast(this, PointerId, Data, bWasFocused);
}

void UUxtTouchTargetComponent::OnUpdateTouchFocus_Implementation(int32 PointerId, const FUxtPointerInteractionData& Data)
{
	FocusedPointers.FindChecked(PointerId) = Data;
	OnUpdateFocus.Broadcast(this, PointerId, Data);
}

void UUxtTouchTargetComponent::OnExitTouchFocus_Implementation(int32 PointerId)
{
	FocusedPointers.Remove(PointerId);
	const bool bIsFocused = FocusedPointers.Num() > 0;
	OnEndFocus.Broadcast(this, PointerId, bIsFocused);
}

bool UUxtTouchTargetComponent::GetClosestTouchPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const
{
	float DistanceSqr;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutPointOnSurface, DistanceSqr);
}

const TMap<int32, FUxtPointerInteractionData>& UUxtTouchTargetComponent::GetFocusedPointers() const
{
	return FocusedPointers;
}
