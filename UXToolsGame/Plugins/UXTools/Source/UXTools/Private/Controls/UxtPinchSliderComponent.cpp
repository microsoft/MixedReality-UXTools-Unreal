// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPinchSliderComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "UXTools.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

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
}

// Sets default values for this component's properties
UUxtPinchSliderComponent::UUxtPinchSliderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	ThumbVisuals.ComponentProperty = TEXT("SliderThumb");
	TrackVisuals.ComponentProperty = TEXT("SliderTrack");
	TickMarkVisuals.ComponentProperty = TEXT("TickMarks");
	SliderStartDistance = 0.0f;
	SliderEndDistance = 50.0f;
	SliderLowerBound = 0.0f;
	SliderUpperBound = 1.0f;
	SliderValue = 0.5f;
	NumTickMarks = 5;
	TickMarkScale = FVector(0.0075f, 0.0075f, 0.0075f);
	CurrentState = EUxtSliderState::Default;
	CollisionProfile = TEXT("UI");
	Smoothing = 50.0f;
	SetComponentTickEnabled(false);
}

void UUxtPinchSliderComponent::SetCollisionProfile(FName Profile)
{
	CollisionProfile = Profile;
	if (BoxComponent)
	{
		BoxComponent->SetCollisionProfileName(CollisionProfile);
	}
}

void UUxtPinchSliderComponent::SetEnabled(bool bEnabled)
{
	if (bEnabled && CurrentState == EUxtSliderState::Disabled)
	{
		bool bWasAlreadyFocused = false;

		for (UUxtFarPointerComponent* FarPointer : FocusingFarPointers)
		{
			OnBeginFocus.Broadcast(this, FarPointer, bWasAlreadyFocused);
			bWasAlreadyFocused = true;
		}

		for (UUxtNearPointerComponent* NearPointer : FocusingNearPointers)
		{
			OnEndFocus.Broadcast(this, NearPointer, bWasAlreadyFocused);
			bWasAlreadyFocused = true;
		}
		
		const bool bIsFocused = FocusingFarPointers.Num() + FocusingNearPointers.Num() > 0;
		CurrentState = bIsFocused ? EUxtSliderState::Focus : EUxtSliderState::Default;

		OnUpdateState.Broadcast(CurrentState);
		OnSliderEnabled.Broadcast(this);
	}
	else if (!bEnabled && CurrentState != EUxtSliderState::Disabled)
	{
		if (GrabPointerWeak.Get())
		{
			GrabPointerWeak->SetFocusLocked(false);
			GrabPointerWeak = nullptr;
		}

		CurrentState = EUxtSliderState::Disabled;
		OnUpdateState.Broadcast(CurrentState);
		OnSliderDisabled.Broadcast(this);
	}
}

#if WITH_EDITOR
void UUxtPinchSliderComponent::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	UpdateSliderState();

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UUxtPinchSliderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdateSliderState();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UUxtPinchSliderComponent::UpdateSliderState()
{
	float BarSize = FMath::Max(SMALL_NUMBER, SliderEndDistance - SliderStartDistance);
	SliderValue = FMath::Clamp(SliderValue, SliderLowerBound, SliderUpperBound);
	UpdateThumbPositionFromSliderValue();

	if (UStaticMeshComponent* Track = GetTrackVisuals())
	{
		FVector Min, Max;
		Track->GetLocalBounds(Min, Max);
		FVector RelativePos = Track->GetRelativeLocation();
		float YOffset = SliderStartDistance + (BarSize / 2.0f);
		Track->SetRelativeLocation(FVector(RelativePos.X, YOffset, RelativePos.Z));
		float YScale = BarSize / (Max.Y - Min.Y);
		FVector RelativeScale = Track->GetRelativeScale3D();
		Track->SetRelativeScale3D(FVector(RelativeScale.X, YScale, RelativeScale.Z));
	}

	if (UInstancedStaticMeshComponent* Ticks = GetTickMarkVisuals())
	{
		Ticks->ClearInstances();
		if (NumTickMarks > 0)
		{
			float Step = NumTickMarks > 1 ? BarSize / (float(NumTickMarks) - 1.0f) : BarSize;
			FTransform T = FTransform::Identity;

			if (NumTickMarks == 1)
			{
				//if its the first get the midpoint.
				T.SetTranslation(FVector(0.0f, (BarSize / 2) + SliderStartDistance, 0.0f));
				Ticks->AddInstance(T);
				T.AddToTranslation(FVector(0.0f, Step, 0.0f));
			}
			else
			{
				T.SetTranslation(FVector(0.0f, SliderStartDistance, 0.0f));
				T.SetScale3D(TickMarkScale);
				for (int i = 0; i < NumTickMarks; ++i)
				{
					Ticks->AddInstance(T);
					T.AddToTranslation(FVector(0.0f, Step, 0.0f));
				}
			}
		}

	}
}

void UUxtPinchSliderComponent::BeginFocus(UUxtPointerComponent* Pointer)
{
	if (CurrentState != EUxtSliderState::Disabled)
	{
		// One pointer means we just received focus
		const bool bWasAlreadyFocused = FocusingFarPointers.Num() + FocusingNearPointers.Num() != 1;

		if (!bWasAlreadyFocused)
		{
			CurrentState = EUxtSliderState::Focus;
			OnUpdateState.Broadcast(CurrentState);
		}

		OnBeginFocus.Broadcast(this, Pointer, bWasAlreadyFocused);
	}
}

void UUxtPinchSliderComponent::EndFocus(UUxtPointerComponent* Pointer)
{
	if (CurrentState != EUxtSliderState::Disabled)
	{
		const bool bIsStillFocused = FocusingFarPointers.Num() + FocusingNearPointers.Num() > 0;

		if (!bIsStillFocused)
		{
			CurrentState = EUxtSliderState::Default;
			OnUpdateState.Broadcast(CurrentState);
		}

		OnEndFocus.Broadcast(this, Pointer, bIsStillFocused);
	}
}

void UUxtPinchSliderComponent::BeginGrab(UUxtPointerComponent* Pointer)
{
	GrabPointerWeak = Pointer;
	Pointer->SetFocusLocked(true);

	CurrentState = EUxtSliderState::Grab;
	OnUpdateState.Broadcast(CurrentState);
	OnBeginInteraction.Broadcast(this, Pointer);

	if (const UStaticMeshComponent* Thumb = GetThumbVisuals())
	{
		GrabThumbStartPositionLS = Thumb->GetRelativeLocation().Y;
	}
}

void UUxtPinchSliderComponent::UpdateGrab(FVector DeltaPos)
{
	DeltaPos = GetComponentTransform().InverseTransformVector(DeltaPos);

	const float NewLocation = GrabThumbStartPositionLS + DeltaPos.Y;
	UpdateSliderValueFromLocalPosition(NewLocation);
}

void UUxtPinchSliderComponent::EndGrab(UUxtPointerComponent* Pointer)
{
	GrabPointerWeak = nullptr;
	Pointer->SetFocusLocked(false);

	const bool bIsFocused = FocusingFarPointers.Num() + FocusingNearPointers.Num() > 0;
	CurrentState = bIsFocused ? EUxtSliderState::Focus : EUxtSliderState::Default;
	OnUpdateState.Broadcast(CurrentState);
	OnEndInteraction.Broadcast(this, Pointer);
}

UStaticMeshComponent* UUxtPinchSliderComponent::GetThumbVisuals() const
{
	return Cast<UStaticMeshComponent>(ThumbVisuals.GetComponent(GetOwner()));
}

UStaticMeshComponent* UUxtPinchSliderComponent::GetTrackVisuals() const
{
	return Cast<UStaticMeshComponent>(TrackVisuals.GetComponent(GetOwner()));
}

UInstancedStaticMeshComponent* UUxtPinchSliderComponent::GetTickMarkVisuals() const
{
	return Cast<UInstancedStaticMeshComponent>(TickMarkVisuals.GetComponent(GetOwner()));
}

void UUxtPinchSliderComponent::SetThumbVisuals(UStaticMeshComponent* Visuals)
{
	ThumbVisuals.OverrideComponent = Visuals;
	if (Visuals)
	{
		Visuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ConfigureBoxComponent(Visuals);
	}
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetTrackVisuals(UStaticMeshComponent* Visuals)
{
	TrackVisuals.OverrideComponent = Visuals;
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetTickMarkVisuals(UInstancedStaticMeshComponent* Visuals)
{
	TickMarkVisuals.OverrideComponent = Visuals;
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetSliderStartDistance(float Distance)
{
	SliderStartDistance = Distance;
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetSliderEndDistance(float Distance)
{
	SliderEndDistance = Distance;
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetSliderLowerBound(float NewBound)
{
	SliderLowerBound = FMath::Clamp(NewBound, 0.0f, 1.0f);
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetSliderUpperBound(float NewBound)
{
	SliderUpperBound = FMath::Clamp(NewBound, 0.0f, 1.0f);
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetSliderValue(float NewValue)
{
	SliderValue = FMath::Clamp(NewValue, SliderLowerBound, SliderUpperBound);
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetNumTickMarks(int Ticks)
{
	NumTickMarks = Ticks;
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetTickMarkScale(FVector Scale)
{
	TickMarkScale = Scale;
	UpdateSliderState();
}

void UUxtPinchSliderComponent::SetSmoothing(float NewSmoothing)
{
	Smoothing = FMath::Max(NewSmoothing, 0.0f);
}

void UUxtPinchSliderComponent::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent = NewObject<UBoxComponent>(this);

	BoxComponent->SetupAttachment(this);
	BoxComponent->RegisterComponent();

	if (UStaticMeshComponent* Visuals = GetThumbVisuals())
	{

		Visuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ConfigureBoxComponent(Visuals);
	}
}

void UUxtPinchSliderComponent::UpdateSliderValueFromLocalPosition(float LocalValue)
{
	const float BarSize = FMath::Max(SMALL_NUMBER, SliderEndDistance - SliderStartDistance);
	const float NewSliderValue = FMath::Clamp((LocalValue - SliderStartDistance) / BarSize, SliderLowerBound, SliderUpperBound);
	SliderValue = SmoothValue(SliderValue, NewSliderValue, Smoothing, GetWorld()->GetDeltaSeconds());

	OnUpdateValue.Broadcast(this, SliderValue);
	UpdateThumbPositionFromSliderValue();
}

void UUxtPinchSliderComponent::UpdateThumbPositionFromSliderValue()
{
	UStaticMeshComponent* Thumb = GetThumbVisuals();
	if (Thumb)
	{
		FVector RelativePos = Thumb->GetRelativeLocation();
		Thumb->SetRelativeLocation(FVector(RelativePos.X, FMath::Lerp(SliderStartDistance, SliderEndDistance, SliderValue), RelativePos.Z));
		if (BoxComponent)
		{
			BoxComponent->SetRelativeLocation(FVector(RelativePos.X, FMath::Lerp(SliderStartDistance, SliderEndDistance, SliderValue), RelativePos.Z));
		}
	}
}

void UUxtPinchSliderComponent::ConfigureBoxComponent(const UStaticMeshComponent* Mesh)
{
	if (!BoxComponent)
	{
		UE_LOG(UXTools, Error, TEXT("Attempting to configure the box component for '%s' before it is initialised, the slider will not work properly."), *GetOwner()->GetName());
		return;
	}

	FVector Min, Max;
	Mesh->GetLocalBounds(Min, Max);

	BoxComponent->SetBoxExtent((Max - Min) * 0.5f);
	BoxComponent->SetWorldTransform(FTransform((Max + Min) / 2) * Mesh->GetComponentTransform());
	BoxComponent->SetCollisionProfileName(CollisionProfile);
}

void UUxtPinchSliderComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && CurrentState == EUxtSliderState::Grab)
	{
		EndGrab(Pointer);
	}
}

void UUxtPinchSliderComponent::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && CurrentState == EUxtSliderState::Grab)
	{
		FVector DeltaPos = Pointer->GetPointerOrigin() - GrabStartPositionWS;
		UpdateGrab(DeltaPos);
	}
}

void UUxtPinchSliderComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && CurrentState != EUxtSliderState::Disabled && CurrentState != EUxtSliderState::Grab)
	{
		GrabStartPositionWS = Pointer->GetPointerOrigin();
		BeginGrab(Pointer);
	}
}

void UUxtPinchSliderComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	FocusingFarPointers.Remove(Pointer);
	EndFocus(Pointer);
}

void UUxtPinchSliderComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	FocusingFarPointers.Add(Pointer);
	BeginFocus(Pointer);
}

bool UUxtPinchSliderComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return BoxComponent == Primitive;
}

void UUxtPinchSliderComponent::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && CurrentState == EUxtSliderState::Grab)
	{
		EndGrab(Pointer);
	}
}

void UUxtPinchSliderComponent::OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && CurrentState == EUxtSliderState::Grab)
	{
		FVector DeltaPos = Pointer->GetGrabPointerTransform().GetLocation() - GrabStartPositionWS;
		UpdateGrab(DeltaPos);
	}
}

void UUxtPinchSliderComponent::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && CurrentState != EUxtSliderState::Disabled && CurrentState != EUxtSliderState::Grab)
	{
		GrabStartPositionWS = Pointer->GetGrabPointerTransform().GetLocation();
		BeginGrab(Pointer);
	}
}

void UUxtPinchSliderComponent::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	FocusingNearPointers.Remove(Pointer);
	EndFocus(Pointer);
}

void UUxtPinchSliderComponent::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	FocusingNearPointers.Add(Pointer);
	BeginFocus(Pointer);
}

bool UUxtPinchSliderComponent::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return BoxComponent == Primitive;
}
