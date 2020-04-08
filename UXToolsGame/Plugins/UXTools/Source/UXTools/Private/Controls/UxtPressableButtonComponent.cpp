// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableButtonComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "UXTools.h"
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
		if (UStaticMeshComponent* Touchable = Cast<UStaticMeshComponent>(GetVisuals()))
		{
			Touchable->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ConfigureBoxComponent(Touchable);
		}

		const FVector VisualsOffset = Visuals->GetComponentLocation() - RestPosition;
		VisualsOffsetLocal = GetComponentTransform().InverseTransformVector(VisualsOffset);
	}
}


void UUxtPressableButtonComponent::SetCollisionProfile(FName Profile)
{
	CollisionProfile = Profile;
	if (BoxComponent)
	{
		BoxComponent->SetCollisionProfileName(CollisionProfile);
	}
}

bool UUxtPressableButtonComponent::IsPressed() const
{
	return bIsPressed;
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


	if (USceneComponent* Visuals = GetVisuals())
	{
		if (UStaticMeshComponent* Pokable = Cast<UStaticMeshComponent>(Visuals))
		{
			Pokable->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			ConfigureBoxComponent(Pokable);
		}

		const FVector VisualsOffset = Visuals->GetComponentLocation() - RestPosition;
		VisualsOffsetLocal = GetComponentTransform().InverseTransformVector(VisualsOffset);
	}
}

// Called every frame
void UUxtPressableButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update poke if we're not currently pressed via a far pointer
	if (!FarPointerWeak.IsValid())
	{
		// Update button logic with all known pointers
		UUxtNearPointerComponent* NewPokingPointer = nullptr;
		float TargetDistance = 0;

		for (UUxtNearPointerComponent* Pointer : PokePointers)
		{
			float PushDistance = CalculatePushDistance(Pointer);
			if (PushDistance > TargetDistance)
			{
				NewPokingPointer = Pointer;
				TargetDistance = PushDistance;
			}
		}

		check(TargetDistance >= 0 && TargetDistance <= GetScaleAdjustedMaxPushDistance());

		const float PreviousPushDistance = CurrentPushDistance;

		// Update push distance and raise events
		if (TargetDistance > CurrentPushDistance)
		{
			CurrentPushDistance = TargetDistance;
			float PressedDistance = GetPressedDistance();

			if (!bIsPressed && CurrentPushDistance >= PressedDistance && PreviousPushDistance < PressedDistance)
			{
				bIsPressed = true;
				OnButtonPressed.Broadcast(this);
			}
		}
		else
		{
			CurrentPushDistance = FMath::Max(TargetDistance, CurrentPushDistance - DeltaTime * RecoverySpeed);
			float ReleasedDistance = GetReleasedDistance();

			// Raise button released if we're pressed and crossed the released distance
			if (bIsPressed && (CurrentPushDistance <= ReleasedDistance && PreviousPushDistance > ReleasedDistance))
			{
				bIsPressed = false;
				OnButtonReleased.Broadcast(this);
			}
		}
	}

	// Update visuals position
	if (USceneComponent* Visuals = GetVisuals())
	{
		const FVector VisualsOffset = GetComponentTransform().TransformVector(VisualsOffsetLocal);
		FVector NewVisualsLocation = VisualsOffset + GetCurrentButtonLocation();
		Visuals->SetWorldLocation(NewVisualsLocation);

		const FVector ColliderOffset = GetComponentTransform().TransformVector(ColliderOffsetLocal);
		FVector NewColliderLocation = ColliderOffset + GetCurrentButtonLocation();
		BoxComponent->SetWorldLocation(NewColliderLocation);
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
		for (const auto& Pointer : GetPokePointers())
		{
			auto Position = Pointer.Key->GetPokePointerTransform().GetLocation();

			// Shift it up a bit so it is not hidden by the pointer visuals.
			Position.Z += 2;

			DrawDebugPoint(GetWorld(), Position, 10, FColor::Yellow);
		}
	}
#endif
}

void UUxtPressableButtonComponent::OnEnterFocus(UObject* Pointer)
{
	const bool bWasFocused = ++NumPointersFocusing > 1;
	OnBeginFocus.Broadcast(this, Pointer, bWasFocused);
}

void UUxtPressableButtonComponent::OnExitFocus(UObject* Pointer)
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

bool UUxtPressableButtonComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return Primitive == BoxComponent;
}

void UUxtPressableButtonComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtPressableButtonComponent::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

void UUxtPressableButtonComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtPressableButtonComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	// Lock the poking pointer so we remain the focused target as it moves.
	Pointer->SetFocusLocked(true);

	PokePointers.Add(Pointer);
	OnBeginPoke.Broadcast(this, Pointer);
}

void UUxtPressableButtonComponent::OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnUpdatePoke.Broadcast(this, Pointer);
}

void UUxtPressableButtonComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (bIsPressed && NumPointersFocusing == 0)
	{
		bIsPressed = false;
		OnButtonReleased.Broadcast(this);
	}

	// Unlock the pointer focus so that another target can be selected.
	Pointer->SetFocusLocked(false);

	PokePointers.Remove(Pointer);
	OnEndPoke.Broadcast(this, Pointer);
}

EUxtPokeBehaviour UUxtPressableButtonComponent::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::FrontFace;
}

float UUxtPressableButtonComponent::CalculatePushDistance(const UUxtNearPointerComponent* pointer) const
{
	FVector RayEndLocal;

	// Calculate current pointer position in local space
	{
		const FQuat InvOrientation = BoxComponent->GetComponentQuat().Inverse();
		RayEndLocal = InvOrientation * (pointer->GetPokePointerTransform().GetLocation() - RestPosition);
	}

	const float endDistance = RayEndLocal.X;

	return endDistance > 0 ? FMath::Min(endDistance, GetScaleAdjustedMaxPushDistance()) : 0;
}

FVector UUxtPressableButtonComponent::GetCurrentButtonLocation() const
{
	return RestPosition + (GetComponentTransform().GetUnitAxis(EAxis::X) * CurrentPushDistance);
}

bool UUxtPressableButtonComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return Primitive == BoxComponent;
}

void UUxtPressableButtonComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtPressableButtonComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}


float UUxtPressableButtonComponent::GetPressedDistance() const
{
	return GetScaleAdjustedMaxPushDistance() * PressedFraction;
}


float UUxtPressableButtonComponent::GetReleasedDistance() const
{
	return GetScaleAdjustedMaxPushDistance() * ReleasedFraction;
}

void UUxtPressableButtonComponent::ConfigureBoxComponent(const UStaticMeshComponent* Mesh)
{
	if (!BoxComponent)
	{
		UE_LOG(UXTools, Error, TEXT("Attempting to configure the box component for '%s' before it is initialised, the button will not work properly."), *GetOwner()->GetName());
		return;
	}

	FVector Min, Max;
	Mesh->GetLocalBounds(Min, Max);

	BoxComponent->SetBoxExtent((Max - Min) * 0.5f);

	BoxComponent->SetWorldTransform(FTransform((Max + Min) / 2) * Mesh->GetComponentTransform());

	BoxComponent->SetCollisionProfileName(CollisionProfile);

	RestPosition = BoxComponent->GetComponentLocation();
	RestPosition.X -= BoxComponent->GetScaledBoxExtent().X;

	const FVector ColliderOffset = BoxComponent->GetComponentLocation() - RestPosition;
	ColliderOffsetLocal = GetComponentTransform().InverseTransformVector(ColliderOffset);
}

void UUxtPressableButtonComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtPressableButtonComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!FarPointerWeak.IsValid())
	{
		CurrentPushDistance = GetPressedDistance();
		FarPointerWeak = Pointer;
		Pointer->SetFocusLocked(true);
		OnButtonPressed.Broadcast(this);
	}
}

void UUxtPressableButtonComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get();
	if (Pointer == FarPointer)
	{
		CurrentPushDistance = 0;
		FarPointerWeak = nullptr;
		Pointer->SetFocusLocked(false);
		OnButtonReleased.Broadcast(this);
	}
}
