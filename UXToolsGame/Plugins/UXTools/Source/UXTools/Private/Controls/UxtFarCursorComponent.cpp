// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFarCursorComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "GameFramework/Actor.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UXTools.h"


UUxtFarCursorComponent::UUxtFarCursorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	RingThickness = 0.64f;
	BorderThickness = 0.08f;

	// Will start ticking when the far pointer is enabled
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetHiddenInGame(true);
}

void UUxtFarCursorComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	UUxtFarPointerComponent* FarPointer = Owner->FindComponentByClass<UUxtFarPointerComponent>();

	if (FarPointer)
	{
		FarPointerWeak = FarPointer;

		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(FarPointer);

		// Activate now if the pointer is enabled
		if (FarPointer->IsEnabled())
		{
			OnFarPointerEnabled(FarPointer);
		}

		// Subscribe to state changes
		FarPointer->OnFarPointerEnabled.AddDynamic(this, &UUxtFarCursorComponent::OnFarPointerEnabled);
		FarPointer->OnFarPointerDisabled.AddDynamic(this, &UUxtFarCursorComponent::OnFarPointerDisabled);
	}
	else
	{
		UE_LOG(UXTools, Error, TEXT("Could not find a far pointer in actor '%s'. Far cursor won't work properly."), *Owner->GetName());
	}
}

void UUxtFarCursorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		FarPointer->OnFarPointerEnabled.RemoveDynamic(this, &UUxtFarCursorComponent::OnFarPointerEnabled);
		FarPointer->OnFarPointerDisabled.RemoveDynamic(this, &UUxtFarCursorComponent::OnFarPointerDisabled);
	}
}

void UUxtFarCursorComponent::OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer)
{
	SetActive(true);
	SetHiddenInGame(false);
}

void UUxtFarCursorComponent::OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer)
{
	SetActive(false);
	SetHiddenInGame(true);
	SetPressed(false);
}

void UUxtFarCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		// Place hovering the hit location
		const auto HitNormal = FarPointer->GetHitNormal();
		FVector Location = FarPointer->GetHitPoint() + HitNormal * HoverDistance;
		SetWorldLocation(Location);

		// Align with hit normal
		const FMatrix Rotation = FRotationMatrix::MakeFromX(-HitNormal);
		SetWorldRotation(Rotation.ToQuat());

		// Update pressed state
		SetPressed(FarPointer->IsPressed());

		// Scale with distance to head
		float DistanceToCamera = (UUxtFunctionLibrary::GetHeadPose(this).GetTranslation() - Location).Size();
		float ReferenceDistance = 100.0f;
		float NewRadius = bPressed ? PressedRadius : IdleRadius;
		NewRadius *= DistanceToCamera / ReferenceDistance;
		SetRadius(NewRadius);
	}
}

void UUxtFarCursorComponent::SetPressed(bool bNewPressed)
{
	if (bNewPressed != bPressed)
	{
		bPressed = bNewPressed;

		if (bPressed)
		{
			// Cache current thickness so we can restore it when released
			IdleRingThickness = GetRingThickness();

			SetRingThickness(1.0f);
		}
		else
		{
			SetRingThickness(IdleRingThickness);
		}
	}
}
