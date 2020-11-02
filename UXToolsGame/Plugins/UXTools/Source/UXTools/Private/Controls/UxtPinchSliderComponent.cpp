// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPinchSliderComponent.h"

#include "UXTools.h"

#include "Components/BoxComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"

namespace
{
	/**
	 * Smooth a value to remove jittering.
	 * Returns a exponentially weighted average of the current start value and the end value based on the time step.
	 */
	float SmoothValue(float StartValue, float EndValue, float Smoothing, float DeltaSeconds)
	{
		if (Smoothing <= 0.0f)
		{
			return EndValue;
		}

		const float Weight = FMath::Clamp(FMath::Exp(-Smoothing * DeltaSeconds), 0.0f, 1.0f);

		return FMath::Lerp(StartValue, EndValue, Weight);
	}
} // namespace

void UUxtPinchSliderComponent::SetEnabled(bool bEnabled)
{
	if (bEnabled && State == EUxtSliderState::Disabled)
	{
		const bool bIsFocused = FocusingPointers.Num() > 0;
		SetState(bIsFocused ? EUxtSliderState::Focused : EUxtSliderState::Default);
		OnEnable.Broadcast(this);

		bool bWasFocused = false;
		for (UUxtPointerComponent* Pointer : FocusingPointers)
		{
			OnBeginFocus.Broadcast(this, Pointer, bWasFocused);
			bWasFocused = true;
		}
	}
	else if (!bEnabled && State != EUxtSliderState::Disabled)
	{
		if (GrabPointer)
		{
			GrabPointer->SetFocusLocked(false);
			GrabPointer = nullptr;
		}

		int NumFocusingPointers = FocusingPointers.Num();
		for (UUxtPointerComponent* Pointer : FocusingPointers)
		{
			OnEndFocus.Broadcast(this, Pointer, NumFocusingPointers != 1);
			--NumFocusingPointers;
		}

		SetState(EUxtSliderState::Disabled);
		OnDisable.Broadcast(this);
	}
}

void UUxtPinchSliderComponent::SetVisuals(UStaticMeshComponent* NewVisuals)
{
	if (NewVisuals)
	{
		Visuals.OverrideComponent = NewVisuals;
		ConfigureBoxComponent();
		UpdateVisuals();
	}
}

void UUxtPinchSliderComponent::SetVisuals(const FComponentReference& NewVisuals)
{
	Visuals = NewVisuals;
	ConfigureBoxComponent();
	UpdateVisuals();
}

void UUxtPinchSliderComponent::SetValue(float NewValue)
{
	Value = FMath::Clamp(NewValue, ValueLowerBound, ValueUpperBound);
	UpdateVisuals();
}

void UUxtPinchSliderComponent::SetTrackLength(float NewTrackLength)
{
	TrackLength = FMath::Max(0.0f, NewTrackLength);
	UpdateVisuals();
}

void UUxtPinchSliderComponent::SetValueLowerBound(float NewLowerBound)
{
	ValueLowerBound = FMath::Clamp(NewLowerBound, 0.0f, 1.0f);
	Value = FMath::Max(Value, ValueLowerBound);
}

void UUxtPinchSliderComponent::SetValueUpperBound(float NewUpperBound)
{
	ValueUpperBound = FMath::Clamp(NewUpperBound, 0.0f, 1.0f);
	Value = FMath::Min(Value, ValueUpperBound);
}

void UUxtPinchSliderComponent::SetUseSteppedMovement(bool bNewUseSteppedMovement)
{
	bUseSteppedMovement = bNewUseSteppedMovement;
}

void UUxtPinchSliderComponent::SetNumSteps(int NewNumSteps)
{
	if (!bUseSteppedMovement)
	{
		UE_LOG(UXTools, Warning, TEXT("The number of steps was set but the slider is using smooth movement."));
	}

	NumSteps = FMath::Max(2, NewNumSteps);
}

void UUxtPinchSliderComponent::SetSmoothing(float NewSmoothing)
{
	if (bUseSteppedMovement)
	{
		UE_LOG(UXTools, Warning, TEXT("The smoothing value was set but the slider is using stepped movement."));
	}

	Smoothing = FMath::Max(0.0f, NewSmoothing);
}

void UUxtPinchSliderComponent::SetCollisionProfile(FName NewCollisionProfile)
{
	CollisionProfile = NewCollisionProfile;
	ConfigureBoxComponent();
}

void UUxtPinchSliderComponent::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent = NewObject<UBoxComponent>(this);
	BoxComponent->SetupAttachment(this);
	BoxComponent->RegisterComponent();

	ConfigureBoxComponent();
	UpdateVisuals();
}

#if WITH_EDITOR
void UUxtPinchSliderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateVisuals();
}
#endif

bool UUxtPinchSliderComponent::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

bool UUxtPinchSliderComponent::CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

void UUxtPinchSliderComponent::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	BeginFocus(Pointer);
}

void UUxtPinchSliderComponent::OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	UpdateFocus(Pointer);
}

void UUxtPinchSliderComponent::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	EndFocus(Pointer);
}

void UUxtPinchSliderComponent::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && State == EUxtSliderState::Focused)
	{
		HandStartPosition = Pointer->GetGrabPointerTransform().GetLocation();
		BeginGrab(Pointer);
	}
}

void UUxtPinchSliderComponent::OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer == GrabPointer && State == EUxtSliderState::Grabbed)
	{
		UpdateGrab(Pointer->GetGrabPointerTransform().GetLocation() - HandStartPosition);
	}
}

void UUxtPinchSliderComponent::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	EndGrab(Pointer);
}

bool UUxtPinchSliderComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

bool UUxtPinchSliderComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return BoxComponent == Primitive;
}

void UUxtPinchSliderComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	BeginFocus(Pointer);
}

void UUxtPinchSliderComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	UpdateFocus(Pointer);
}

void UUxtPinchSliderComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	EndFocus(Pointer);
}

void UUxtPinchSliderComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && State == EUxtSliderState::Focused)
	{
		HandStartPosition = Pointer->GetPointerOrigin();
		BeginGrab(Pointer);
	}
}

void UUxtPinchSliderComponent::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && State == EUxtSliderState::Grabbed)
	{
		UpdateGrab(Pointer->GetPointerOrigin() - HandStartPosition);
	}
}

void UUxtPinchSliderComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	EndGrab(Pointer);
}

void UUxtPinchSliderComponent::BeginFocus(UUxtPointerComponent* Pointer)
{
	if (Pointer)
	{
		FocusingPointers.Add(Pointer);

		if (State != EUxtSliderState::Disabled)
		{
			const bool bWasAlreadyFocused = FocusingPointers.Num() > 1;

			if (!bWasAlreadyFocused)
			{
				SetState(EUxtSliderState::Focused);
			}

			OnBeginFocus.Broadcast(this, Pointer, bWasAlreadyFocused);
		}
	}
}

void UUxtPinchSliderComponent::UpdateFocus(UUxtPointerComponent* Pointer)
{
	if (Pointer && State != EUxtSliderState::Disabled)
	{
		OnUpdateFocus.Broadcast(this, Pointer);
	}
}

void UUxtPinchSliderComponent::EndFocus(UUxtPointerComponent* Pointer)
{
	if (Pointer)
	{
		FocusingPointers.Remove(Pointer);

		if (State != EUxtSliderState::Disabled)
		{
			const bool bIsStillFocused = FocusingPointers.Num() > 0;

			if (!bIsStillFocused)
			{
				SetState(EUxtSliderState::Default);
			}

			OnEndFocus.Broadcast(this, Pointer, bIsStillFocused);
		}
	}
}

void UUxtPinchSliderComponent::BeginGrab(UUxtPointerComponent* Pointer)
{
	if (Pointer && State == EUxtSliderState::Focused)
	{
		GrabPointer = Pointer;
		GrabPointer->SetFocusLocked(true);

		SetState(EUxtSliderState::Grabbed);
		OnBeginGrab.Broadcast(this, GrabPointer);

		if (const UStaticMeshComponent* Thumb = GetVisuals())
		{
			SliderStartPosition = Thumb->GetRelativeLocation().Y;
		}
	}
}

void UUxtPinchSliderComponent::UpdateGrab(FVector DeltaPosition)
{
	if (State == EUxtSliderState::Grabbed)
	{
		const FVector LocalDeltaPosition = GetComponentTransform().InverseTransformVector(DeltaPosition);
		const float HalfTrackLength = TrackLength / 2.0f;
		const float NewValue = 1 - ((SliderStartPosition + LocalDeltaPosition.Y + HalfTrackLength) / TrackLength);

		if (bUseSteppedMovement)
		{
			const float NumZones = NumSteps - 1;
			const float Zone = NewValue * NumZones;
			const float ZoneCeil = FMath::CeilToFloat(Zone) / NumZones;
			const float ZoneFloor = FMath::FloorToFloat(Zone) / NumZones;

			const float DistanceToCeil = ZoneCeil - NewValue;
			const float DistanceToFloor = NewValue - ZoneFloor;
			const float SteppedValue = DistanceToCeil < DistanceToFloor ? ZoneCeil : ZoneFloor;

			SetValue(SteppedValue);
		}
		else
		{
			SetValue(SmoothValue(Value, NewValue, Smoothing, GetWorld()->GetDeltaSeconds()));
		}

		UpdateVisuals();
		OnUpdateValue.Broadcast(this, Value);
	}
}

void UUxtPinchSliderComponent::EndGrab(UUxtPointerComponent* Pointer)
{
	if (Pointer == GrabPointer && State == EUxtSliderState::Grabbed)
	{
		GrabPointer->SetFocusLocked(false);
		GrabPointer = nullptr;

		const bool bIsFocused = FocusingPointers.Num() > 0;
		SetState(bIsFocused ? EUxtSliderState::Focused : EUxtSliderState::Default);
		OnEndGrab.Broadcast(this, Pointer);
	}
}

void UUxtPinchSliderComponent::SetState(EUxtSliderState NewState)
{
	if (NewState != State)
	{
		State = NewState;
		OnUpdateState.Broadcast(this, State);
	}
}

void UUxtPinchSliderComponent::ConfigureBoxComponent()
{
	if (BoxComponent)
	{
		// If the actor's collision is disabled, we need to enable it before disabling collision on the primitive components.
		// This is because the it will restore the previous collision state when re-enabled (i.e. re-enabling collision on the components)
		const bool bActorCollisionDisabled = !GetOwner()->GetActorEnableCollision();
		if (bActorCollisionDisabled)
		{
			GetOwner()->SetActorEnableCollision(true);
		}

		// Disable collision on all primitive components.
		TArray<UPrimitiveComponent*> Components;
		GetOwner()->GetComponents<UPrimitiveComponent>(Components);
		for (UPrimitiveComponent* Component : Components)
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// Match the box to the thumb visuals.
		if (UStaticMeshComponent* Thumb = GetVisuals())
		{
			FVector Min, Max;
			Thumb->GetLocalBounds(Min, Max);

			BoxComponent->SetBoxExtent((Max - Min) * 0.5f);
			BoxComponent->SetWorldTransform(FTransform((Max + Min) / 2) * Thumb->GetComponentTransform());
			BoxComponent->SetCollisionProfileName(CollisionProfile);
		}

		// Disable the actor's collision if we enabled it earlier.
		if (bActorCollisionDisabled)
		{
			GetOwner()->SetActorEnableCollision(false);
		}
	}
}

void UUxtPinchSliderComponent::UpdateVisuals()
{
	if (UStaticMeshComponent* Thumb = GetVisuals())
	{
		const FVector ThumbPosition = Thumb->GetRelativeLocation();
		const float HalfTrackLength = TrackLength / 2.0f;
		const FVector NewThumbPosition(ThumbPosition.X, FMath::Lerp(HalfTrackLength, -HalfTrackLength, Value), ThumbPosition.Z);
		Thumb->SetRelativeLocation(NewThumbPosition);

		if (BoxComponent)
		{
			BoxComponent->SetRelativeLocation(NewThumbPosition);
		}
	}
}
