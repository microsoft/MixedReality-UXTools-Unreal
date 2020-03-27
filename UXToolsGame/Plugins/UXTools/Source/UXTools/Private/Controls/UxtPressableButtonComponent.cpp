// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableButtonComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"

#include <GameFramework/Actor.h>
#include <DrawDebugHelpers.h>
#include <Components/BoxComponent.h>
#include <Components/ShapeComponent.h>
#include <Components/StaticMeshComponent.h>

// Sets default values for this component's properties
UUxtPressableButtonComponent::UUxtPressableButtonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	MaxPushDistance = 10;
	PressedFraction = 0.5f;
	ReleasedFraction = 0.2f;
	RecoverySpeed = 50;
}

USceneComponent* UUxtPressableButtonComponent::GetVisuals() const 
{
	return Cast<USceneComponent>(VisualsReference.GetComponent(GetOwner()));
}

void UUxtPressableButtonComponent::SetVisuals(USceneComponent* Visuals)
{
	VisualsReference.OverrideComponent = Visuals;

	if (Visuals)
	{
		const auto VisualsOffset = Visuals->GetComponentLocation() - GetComponentLocation();
		VisualsOffsetLocal = GetComponentTransform().InverseTransformVector(VisualsOffset);
	}
}

bool UUxtPressableButtonComponent::IsPressed() const
{
	return bIsPressed;
}


FVector2D UUxtPressableButtonComponent::GetButtonExtents() const
{
	if (auto Touchable = Cast<UStaticMeshComponent>(GetVisuals()))
	{
		FVector Min, Max;
		Touchable->GetLocalBounds(Min, Max);

		FVector Extents = (Max - Min) * 0.5f;
		Extents *= Touchable->GetComponentTransform().GetScale3D();

		return FVector2D(Extents.Y, Extents.Z);
	}
	return FVector2D::UnitVector;
}


float UUxtPressableButtonComponent::GetScaleAdjustedMaxPushDistance() const
{
	return MaxPushDistance * GetComponentTransform().GetScale3D().X;
}

// Called when the game starts
void UUxtPressableButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent = NewObject<UBoxComponent>(this);

	BoxComponent->SetupAttachment(this);
	BoxComponent->RegisterComponent();

	if (auto Touchable = Cast<UStaticMeshComponent>(GetVisuals()))
	{
		Touchable->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		FVector Min, Max;
		Touchable->GetLocalBounds(Min, Max);

		BoxComponent->SetBoxExtent((Max - Min) * 0.5f);

		BoxComponent->SetWorldTransform(FTransform((Max + Min) / 2) * Touchable->GetComponentTransform());

		BoxComponent->SetCollisionProfileName(CollisionProfile);

		const auto ColliderOffset = BoxComponent->GetComponentLocation() - GetComponentLocation();
		ColliderOffsetLocal = GetComponentTransform().InverseTransformVector(ColliderOffset);
	}

	const FTransform& Transform = GetComponentTransform();

	RestPosition = Transform.GetTranslation();
	PressedDistance = GetScaleAdjustedMaxPushDistance() * PressedFraction;
	ReleasedDistance = GetScaleAdjustedMaxPushDistance() * ReleasedFraction;

	if (auto Visuals = GetVisuals())
	{
		const auto VisualsOffset = Visuals->GetComponentLocation() - GetComponentLocation();
		VisualsOffsetLocal = GetComponentTransform().InverseTransformVector(VisualsOffset);
	}
}

// Called every frame
void UUxtPressableButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update touch if we're not currently pressed via a far pointer
	if (!FarPointerWeak.IsValid())
	{
		// Update button logic with all known pointers
		UUxtNearPointerComponent* NewTouchingPointer = nullptr;
		float targetDistance = 0;

		for (const auto& Pointer : TouchPointers)
		{
			float pushDistance = CalculatePushDistance(Pointer.Key);
			if (pushDistance > targetDistance)
			{
				NewTouchingPointer = Pointer.Key;
				targetDistance = pushDistance;
			}
		}

		check(targetDistance >= 0 && targetDistance <= GetScaleAdjustedMaxPushDistance());

		const auto previousPushDistance = CurrentPushDistance;

		// Update push distance and raise events
		if (targetDistance > CurrentPushDistance)
		{
			CurrentPushDistance = targetDistance;

			if (!bIsPressed && CurrentPushDistance >= PressedDistance && previousPushDistance < PressedDistance)
			{
				bIsPressed = true;
				OnButtonPressed.Broadcast(this);
			}
		}
		else
		{
			CurrentPushDistance = FMath::Max(targetDistance, CurrentPushDistance - DeltaTime * RecoverySpeed);

			// Raise button released if we're pressed and crossed the released distance
			if (bIsPressed && (CurrentPushDistance <= ReleasedDistance && previousPushDistance > ReleasedDistance))
			{
				bIsPressed = false;
				OnButtonReleased.Broadcast(this);
			}
		}

		// Update visuals position
		if (auto Visuals = GetVisuals())
		{
			const auto VisualsOffset = GetComponentTransform().TransformVector(VisualsOffsetLocal);
			FVector NewVisualsLocation = VisualsOffset + GetCurrentButtonLocation();
			Visuals->SetWorldLocation(NewVisualsLocation);

			const auto ColliderOffset = GetComponentTransform().TransformVector(ColliderOffsetLocal);
			FVector NewColliderLocation = ColliderOffset + GetCurrentButtonLocation();
			BoxComponent->SetWorldLocation(NewColliderLocation);
		}
	}

#if 0
	// Debug display
	{
		// Button face
		{
			FVector Position = GetCurrentButtonLocation();
			FPlane Plane(Position, -GetComponentTransform().GetUnitAxis(EAxis::X));
			FVector2D HalfExtents = GetButtonExtents();
			DrawDebugSolidPlane(GetWorld(), Plane, Position, HalfExtents, FColor::Blue);
		}

		// Pointers
		for (const auto& Pointer : GetTouchPointers())
		{
			auto Position = Pointer.Key->GetTouchPointerTransform().GetLocation();

			// Shift it up a bit so it is not hidden by the pointer visuals.
			Position.Z += 2;

			DrawDebugPoint(GetWorld(), Position, 10, FColor::Yellow);
		}
	}
#endif
}

void UUxtPressableButtonComponent::OnEnterTouchFocus_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	const bool bWasFocused = ++NumPointersFocusing > 1;
	OnBeginFocus.Broadcast(this, Pointer, Data, bWasFocused);
}

void UUxtPressableButtonComponent::OnUpdateTouchFocus_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	OnUpdateFocus.Broadcast(this, Pointer, Data);
}

void UUxtPressableButtonComponent::OnExitTouchFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	const bool bIsFocused = --NumPointersFocusing > 0;

	if (!bIsFocused)
	{
		if (bIsPressed)
		{
			bIsPressed = false;
			OnButtonReleased.Broadcast(this);
		}
	}

	OnEndFocus.Broadcast(this, Pointer, bIsFocused);
}

bool UUxtPressableButtonComponent::GetClosestTouchPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const
{
	float DistanceSqr;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutPointOnSurface, DistanceSqr);
}

void UUxtPressableButtonComponent::OnBeginTouch_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	// Lock the touching pointer so we remain the focused target as it moves.
	Pointer->SetFocusLocked(true);

	TouchPointers.Add(Pointer, Data);
	OnBeginTouch.Broadcast(this, Pointer, Data);
}

void UUxtPressableButtonComponent::OnUpdateTouch_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data)
{
	// Update the copy of the pointer data in the grab pointer array
	TouchPointers.FindChecked(Pointer) = Data;
	OnUpdateTouch.Broadcast(this, Pointer, Data);
}

void UUxtPressableButtonComponent::OnEndTouch_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (bIsPressed && NumPointersFocusing == 0)
	{
		bIsPressed = false;
		OnButtonReleased.Broadcast(this);
	}

	// Unlock the pointer focus so that another target can be selected.
	Pointer->SetFocusLocked(false);

	TouchPointers.Remove(Pointer);
	OnEndTouch.Broadcast(this, Pointer);
}

EUxtTouchBehaviour UUxtPressableButtonComponent::GetTouchBehaviour_Implementation() const
{
	return EUxtTouchBehaviour::FrontFace;
}

float UUxtPressableButtonComponent::CalculatePushDistance(const UUxtNearPointerComponent* pointer) const
{
	FVector RayEndLocal;

	// Calculate current pointer position in local space
	{
		const auto InvOrientation = BoxComponent->GetComponentQuat().Inverse();
		RayEndLocal = InvOrientation * (pointer->GetTouchPointerTransform().GetLocation() - RestPosition);
	}

	const auto endDistance = RayEndLocal.X;

	return endDistance > 0 ? FMath::Min(endDistance, GetScaleAdjustedMaxPushDistance()) : 0;
}

FVector UUxtPressableButtonComponent::GetCurrentButtonLocation() const
{
	return RestPosition + (GetComponentTransform().GetUnitAxis(EAxis::X) * CurrentPushDistance);
}

FVector UUxtPressableButtonComponent::GetVisualsRestPosition() const
{
	const auto& Transform = GetComponentTransform();
	return Transform.GetLocation() + Transform.TransformVector(VisualsOffsetLocal);
}

// TODO.LV
//void UUxtPressableButtonComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent)
//{
//	if (!FarPointerWeak.IsValid())
//	{
//		if (auto Visuals = GetVisuals())
//		{
//			FQuat Orientation = GetComponentTransform().GetRotation();
//			const float PressedDistance = PressedFraction * 2.0f * Extents.X * GetComponentScale().X;
//			Visuals->SetWorldLocation(GetVisualsRestPosition() + Orientation.GetForwardVector() * PressedDistance);
//		}
//
//		FarPointerWeak = Pointer;
//		Pointer->SetFocusLocked(true);
//		OnButtonPressed.Broadcast(this);
//	}
//}
//
//void UUxtPressableButtonComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent)
//{
//	auto FarPointer = FarPointerWeak.Get();
//	if (Pointer == FarPointer)
//	{
//		if (auto Visuals = GetVisuals())
//		{
//			Visuals->SetWorldLocation(GetVisualsRestPosition());
//		}
//
//		OnButtonReleased.Broadcast(this);
//		Pointer->SetFocusLocked(false);
//		FarPointerWeak = nullptr;
//	}
//}
