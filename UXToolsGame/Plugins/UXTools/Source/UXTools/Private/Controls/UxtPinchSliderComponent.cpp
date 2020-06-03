// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPinchSliderComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "UXTools.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
UUxtPinchSliderComponent::UUxtPinchSliderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
	ThumbVisuals.ComponentProperty = TEXT("SliderThumb");
	TrackVisuals.ComponentProperty = TEXT("SliderTrack"); 
	TickMarkVisuals.ComponentProperty = TEXT("TickMarks");
	SliderStartDistance = 0.0f;
	SliderEndDistance = 50.0f;
	SliderValue = 0.5f;
	NumTickMarks = 5;
	TickMarkScale = FVector(0.0075f, 0.0075f, 0.0075f);
	CurrentState = EUxtSliderState::Default;
	CollisionProfile = TEXT("UI");
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
	SliderValue = FMath::Clamp(SliderValue, 0.0f, 1.0f);
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

void UUxtPinchSliderComponent::SetSliderValue(float NewValue)
{
	SliderValue =FMath::Clamp(NewValue,0.0f,1.0f);
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
	float BarSize = FMath::Max(SMALL_NUMBER, SliderEndDistance - SliderStartDistance);
	SliderValue = FMath::Clamp((LocalValue -SliderStartDistance) / BarSize, 0.0f, 1.0f);
	OnValueUpdated.Broadcast(this, SliderValue);
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
		UE_LOG(UXTools, Error, TEXT("Attempting to configure the box component for '%s' before it is initialised, the button will not work properly."), *GetOwner()->GetName());
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
	UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get();
	if (Pointer && Pointer == FarPointer)
	{
		FarPointerWeak = NULL;
		Pointer->SetFocusLocked(false);
		if (CurrentState == EUxtSliderState::Grab)
		{
			OnInteractionEnded.Broadcast(this);
			CurrentState = EUxtSliderState::Default;
			OnStateUpdated.Broadcast(CurrentState);
		}
		
	}
}

void UUxtPinchSliderComponent::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && (CurrentState == EUxtSliderState::Grab))
	{ 
		FVector DeltaPos = Pointer->GetPointerOrigin() - GrabStartPositionWS;
		FTransform T = GetComponentTransform();
		DeltaPos = T.InverseTransformVector(DeltaPos);
		float NewLocation = GrabThumbStartPositionLS + DeltaPos.Y;
		UpdateSliderValueFromLocalPosition(NewLocation);

	}
}

void UUxtPinchSliderComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && (CurrentState != EUxtSliderState::Grab) && !FarPointerWeak.IsValid())
	{
		FarPointerWeak = Pointer;
		Pointer->SetFocusLocked(true);
		OnInteractionStarted.Broadcast(this);
		CurrentState = EUxtSliderState::Grab;
		OnStateUpdated.Broadcast(CurrentState);
		GrabStartPositionWS = Pointer->GetPointerOrigin();
		UStaticMeshComponent* Thumb = GetThumbVisuals();
		if (Thumb)
		{
			GrabThumbStartPositionLS = Thumb->GetRelativeLocation().Y;
		}
		
	}
}

void UUxtPinchSliderComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	const bool bIsFocused = --NumPointersFocusing > 0;

	if (!bIsFocused &&(CurrentState == EUxtSliderState::Focus))
	{
		CurrentState = EUxtSliderState::Default;
		OnStateUpdated.Broadcast(CurrentState);
		OnFocusExit.Broadcast(this);
	}
}


void UUxtPinchSliderComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	++NumPointersFocusing;
	if (CurrentState == EUxtSliderState::Default)
	{
		CurrentState = EUxtSliderState::Focus;
		OnStateUpdated.Broadcast(CurrentState);
		OnFocusEnter.Broadcast(this);
	}
}

bool UUxtPinchSliderComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return BoxComponent == Primitive;
}

void UUxtPinchSliderComponent::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && CurrentState == EUxtSliderState::Grab)
	{
		Pointer->SetFocusLocked(false);
		CurrentState = EUxtSliderState::Default;
		OnStateUpdated.Broadcast(CurrentState);
		OnInteractionEnded.Broadcast(this);
	}	
}

void UUxtPinchSliderComponent::OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && (CurrentState == EUxtSliderState::Grab))
	{
		FVector DeltaPos = Pointer->GetGrabPointerTransform().GetLocation() - GrabStartPositionWS;
		FTransform T = GetComponentTransform();
		DeltaPos = T.InverseTransformVector(DeltaPos);
		float NewLocation = GrabThumbStartPositionLS + DeltaPos.Y;
		UpdateSliderValueFromLocalPosition(NewLocation);	
	}
}

void UUxtPinchSliderComponent::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (Pointer && CurrentState != EUxtSliderState::Grab)
	{
		Pointer->SetFocusLocked(true);
		CurrentState = EUxtSliderState::Grab;
		OnStateUpdated.Broadcast(CurrentState);
		OnInteractionStarted.Broadcast(this);
		GrabStartPositionWS = Pointer->GetGrabPointerTransform().GetLocation();
		UStaticMeshComponent* Thumb = GetThumbVisuals();
		if (Thumb)
		{
			GrabThumbStartPositionLS = Thumb->GetRelativeLocation().Y;
		}
	}
}

void UUxtPinchSliderComponent::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (CurrentState == EUxtSliderState::Focus)
	{
		CurrentState = EUxtSliderState::Default;
		OnStateUpdated.Broadcast(CurrentState);
		OnFocusExit.Broadcast(this);
	}
}


void UUxtPinchSliderComponent::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (CurrentState == EUxtSliderState::Default)
	{
		CurrentState = EUxtSliderState::Focus;
		OnStateUpdated.Broadcast(CurrentState);
		OnFocusEnter.Broadcast(this);
	}
}

bool UUxtPinchSliderComponent::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return BoxComponent == Primitive;
}

