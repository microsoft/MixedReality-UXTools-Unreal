// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtWidgetComponent.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"

#include <Components/PrimitiveComponent.h>
#include <Components/WidgetComponent.h>

namespace
{
	UWidgetComponent* GetNearFocusedWidget(UUxtNearPointerComponent* NearPointer, FVector& OutClosestPoint)
	{
		FVector Unused;
		return Cast<UWidgetComponent>(NearPointer->GetFocusedPokePrimitive(OutClosestPoint, Unused));
	}

	UWidgetComponent* GetFarFocusedWidget(UUxtFarPointerComponent* FarPointer, FVector& OutClosestPoint)
	{
		OutClosestPoint = FarPointer->GetHitPoint();
		return Cast<UWidgetComponent>(FarPointer->GetHitPrimitive());
	}
} // namespace

void UUxtWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	VirtualUser = FSlateApplication::Get().FindOrCreateVirtualUser(VirtualUserIndex);
}

bool UUxtWidgetComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Cast<UWidgetComponent>(Primitive) != nullptr;
}

EUxtPokeBehaviour UUxtWidgetComponent::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::FrontFace;
}

bool UUxtWidgetComponent::GetClosestPoint_Implementation(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	OutNormal = Primitive->GetComponentTransform().GetUnitAxis(EAxis::X);

	float NotUsed;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed);
}

bool UUxtWidgetComponent::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	return Cast<UWidgetComponent>(Primitive) != nullptr;
}

void UUxtWidgetComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetNearFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsPokeFocusable.
	// If this check fails, a non poke focusable object has received focus.
	check(Widget);

	FVector2D LocalHitLocation;
	Widget->GetLocalHitLocation(ClosestPoint, LocalHitLocation);

	Pointers.Add(Pointer, LocalHitLocation);
}

void UUxtWidgetComponent::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetNearFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsPokeFocusable.
	// If this check fails, a non poke focusable object has received focus.
	check(Widget);

	PointerMove(ClosestPoint, Pointer, Widget);
}

void UUxtWidgetComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (bIsPressed && Pointers.Num() == 1)
	{
		FVector ClosestPoint;
		UWidgetComponent* Widget = GetNearFocusedWidget(Pointer, ClosestPoint);

		// Primitive should be castable to widget component as this is a condition in IsPokeFocusable.
		// If this check fails, a non poke focusable object has received focus.
		check(Widget);

		PointerUp(ClosestPoint, Pointer, Widget);
	}

	Pointers.Remove(Pointer);
}

void UUxtWidgetComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetNearFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsPokeFocusable.
	// If this check fails, a non poke focusable object has received focus.
	check(Widget);

	PointerDown(ClosestPoint, Pointer, Widget);
}

void UUxtWidgetComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetNearFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsPokeFocusable.
	// If this check fails, a non poke focusable object has received focus.
	check(Widget);

	PointerUp(ClosestPoint, Pointer, Widget);
}

bool UUxtWidgetComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Cast<UWidgetComponent>(Primitive) != nullptr;
}

bool UUxtWidgetComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return Cast<UWidgetComponent>(Primitive) != nullptr;
}

void UUxtWidgetComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetFarFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsFarFocusable.
	// If this check fails, a non far focusable object has received focus.
	check(Widget);

	FVector2D LocalHitLocation;
	Widget->GetLocalHitLocation(ClosestPoint, LocalHitLocation);

	Pointers.Add(Pointer, LocalHitLocation);
}

void UUxtWidgetComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetFarFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsFarFocusable.
	// If this check fails, a non far focusable object has received focus.
	check(Widget);

	PointerMove(ClosestPoint, Pointer, Widget);
}

void UUxtWidgetComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (bIsPressed && Pointers.Num() == 1)
	{
		FVector ClosestPoint;
		UWidgetComponent* Widget = GetFarFocusedWidget(Pointer, ClosestPoint);

		// Primitive should be castable to widget component as this is a condition in IsFarFocusable.
		// If this check fails, a non far focusable object has received focus.
		check(Widget);

		PointerUp(ClosestPoint, Pointer, Widget);
	}

	Pointers.Remove(Pointer);
}

void UUxtWidgetComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetFarFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsFarFocusable.
	// If this check fails, a non far focusable object has received focus.
	check(Widget);

	PointerDown(ClosestPoint, Pointer, Widget);
}

void UUxtWidgetComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	FVector ClosestPoint;
	UWidgetComponent* Widget = GetFarFocusedWidget(Pointer, ClosestPoint);

	// Primitive should be castable to widget component as this is a condition in IsFarFocusable.
	// If this check fails, a non far focusable object has received focus.
	check(Widget);

	PointerUp(ClosestPoint, Pointer, Widget);
}

void UUxtWidgetComponent::PointerMove(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget)
{
	FPointerEvent Event;
	FWidgetPath Path;
	GetEventAndPath(ClosestPoint, Pointer, Widget, FKey(), Event, Path);

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

void UUxtWidgetComponent::PointerDown(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget)
{
	bIsPressed = true;

	PressedKeys.Add(EKeys::LeftMouseButton);

	FPointerEvent Event;
	FWidgetPath Path;
	GetEventAndPath(ClosestPoint, Pointer, Widget, EKeys::LeftMouseButton, Event, Path);

	FSlateApplication::Get().RoutePointerDownEvent(Path, Event);
}

void UUxtWidgetComponent::PointerUp(const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget)
{
	bIsPressed = false;

	PressedKeys.Remove(EKeys::LeftMouseButton);

	FPointerEvent Event;
	FWidgetPath Path;
	GetEventAndPath(ClosestPoint, Pointer, Widget, EKeys::LeftMouseButton, Event, Path);

	FSlateApplication::Get().RoutePointerUpEvent(Path, Event);
}

void UUxtWidgetComponent::GetEventAndPath(
	const FVector& ClosestPoint, UUxtPointerComponent* Pointer, UWidgetComponent* Widget, FKey Key, FPointerEvent& Event, FWidgetPath& Path)
{
	FVector2D LocalHitLocation;
	Widget->GetLocalHitLocation(ClosestPoint, LocalHitLocation);

	Path = Widget->GetHitWidgetPath(LocalHitLocation, false);

	Event = FPointerEvent(
		VirtualUser->GetUserIndex(), Pointer->GetUniqueID(), LocalHitLocation, Pointers[Pointer], PressedKeys, Key, 0.0f, ModifierKeys);

	Pointers[Pointer] = LocalHitLocation;
}
