// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtWidgetComponent.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"


void UUxtWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	VirtualUser = FSlateApplication::Get().FindOrCreateVirtualUser(VirtualUserIndex);
}

bool UUxtWidgetComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == this;
}

EUxtPokeBehaviour UUxtWidgetComponent::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::FrontFace;
}

bool UUxtWidgetComponent::GetClosestPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	OutNormal = GetComponentTransform().GetUnitAxis(EAxis::X);

	float NotUsed;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed);
}

void UUxtWidgetComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	Pointers.Add(Pointer);

	FVector ClosestPoint, Normal;
	Pointer->GetFocusedPokeTarget(ClosestPoint, Normal);

	FVector2D LocalHitLocation;
	GetLocalHitLocation(ClosestPoint, LocalHitLocation);

	LastLocalHitLocations.Add(Pointer, LocalHitLocation);
}

void UUxtWidgetComponent::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint, Normal;
	Pointer->GetFocusedPokeTarget(ClosestPoint, Normal);

	PointerMove(ClosestPoint, Pointer);
}

void UUxtWidgetComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (bIsPressed && Pointers.Num() == 1)
	{
		FVector ClosestPoint, Normal;
		Pointer->GetFocusedPokeTarget(ClosestPoint, Normal);

		PointerUp(ClosestPoint, Pointer);
	}

	Pointers.Remove(Pointer);
	LastLocalHitLocations.Remove(Pointer);
}

void UUxtWidgetComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint, Normal;
	Pointer->GetFocusedPokeTarget(ClosestPoint, Normal);

	PointerDown(ClosestPoint, Pointer);
}

void UUxtWidgetComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint, Normal;
	Pointer->GetFocusedPokeTarget(ClosestPoint, Normal);

	PointerUp(ClosestPoint, Pointer);
}

bool UUxtWidgetComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return Primitive == this;
}

void UUxtWidgetComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	Pointers.Add(Pointer);

	FVector ClosestPoint = Pointer->GetHitPoint();

	FVector2D LocalHitLocation;
	GetLocalHitLocation(ClosestPoint, LocalHitLocation);

	LastLocalHitLocations.Add(Pointer, LocalHitLocation);
}

void UUxtWidgetComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint = Pointer->GetHitPoint();

	PointerMove(ClosestPoint, Pointer);
}

void UUxtWidgetComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (bIsPressed && Pointers.Num() == 1)
	{
		FVector ClosestPoint = Pointer->GetHitPoint();

		PointerUp(ClosestPoint, Pointer);
	}

	Pointers.Remove(Pointer);
	LastLocalHitLocations.Remove(Pointer);
}

void UUxtWidgetComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint = Pointer->GetHitPoint();

	PointerDown(ClosestPoint, Pointer);
}

void UUxtWidgetComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint = Pointer->GetHitPoint();

	PointerUp(ClosestPoint, Pointer);
}

void UUxtWidgetComponent::PointerMove(const FVector& ClosestPoint, UUxtPointerComponent* Pointer)
{
	FPointerEvent Event;
	FWidgetPath Path;
	GetEventAndPath(ClosestPoint, Pointer, FKey(), Event, Path);

	if (Path.IsValid())
	{
		FSlateApplication::Get().RoutePointerMoveEvent(Path, Event, false);
	}
	else
	{
		FWidgetPath EmptyWidgetPath;
		FSlateApplication::Get().RoutePointerMoveEvent(EmptyWidgetPath, Event, false);
	}
}

void UUxtWidgetComponent::PointerDown(const FVector& ClosestPoint, UUxtPointerComponent* Pointer)
{
	bIsPressed = true;

	PressedKeys.Add(EKeys::LeftMouseButton);

	FPointerEvent Event;
	FWidgetPath Path;
	GetEventAndPath(ClosestPoint, Pointer, EKeys::LeftMouseButton, Event, Path);

	FSlateApplication::Get().RoutePointerDownEvent(Path, Event);
}

void UUxtWidgetComponent::PointerUp(const FVector& ClosestPoint, UUxtPointerComponent* Pointer)
{
	bIsPressed = false;

	PressedKeys.Remove(EKeys::LeftMouseButton);

	FPointerEvent Event;
	FWidgetPath Path;
	GetEventAndPath(ClosestPoint, Pointer, EKeys::LeftMouseButton, Event, Path);

	FSlateApplication::Get().RoutePointerUpEvent(Path, Event);
}

void UUxtWidgetComponent::GetEventAndPath(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, FKey Key, FPointerEvent& Event, FWidgetPath& Path)
{
	FVector2D LocalHitLocation;
	GetLocalHitLocation(ClosestPoint, LocalHitLocation);

	Path = GetHitWidgetPath(LocalHitLocation, false);

	uint32 PointerIndex = Pointers.FindId(Pointer).AsInteger();

	Event = FPointerEvent(
		VirtualUser->GetUserIndex(),
		PointerIndex,
		LocalHitLocation,
		LastLocalHitLocations[Pointer],
		PressedKeys,
		Key,
		0.0f,
		ModifierKeys);

	LastLocalHitLocations[Pointer] = LocalHitLocation;
}
