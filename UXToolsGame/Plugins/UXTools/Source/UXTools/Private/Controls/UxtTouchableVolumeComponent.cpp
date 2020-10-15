// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtTouchableVolumeComponent.h"

#include "UXTools.h"

#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"

#include <Components/PrimitiveComponent.h>
#include <GameFramework/Actor.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>

void UUxtTouchableVolumeComponent::SetEnabled(bool Enabled)
{
	if (Enabled && bIsDisabled)
	{
		bIsDisabled = false;
		OnVolumeEnabled.Broadcast(this);
	}
	else if (!Enabled && !bIsDisabled)
	{
		// Free any locked pointers
		if (FarPointerWeak.Get())
		{
			FarPointerWeak->SetFocusLocked(false);
			FarPointerWeak = nullptr;
		}
		PokePointers.Empty();

		bIsDisabled = true;
		OnVolumeDisabled.Broadcast(this);
	}
}

// Called when the game starts
void UUxtTouchableVolumeComponent::BeginPlay()
{
	Super::BeginPlay();

#if PLATFORM_ANDROID
	bool bEnableTouch = true;
#else
	bool bEnableTouch = GetWorld()->IsPlayInMobilePreview();
#endif

	// Subscribe to touch events raised on primitives
	if (bEnableTouch)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		PlayerController->bEnableTouchEvents = true;
		PlayerController->bEnableTouchOverEvents = true;
		GetOwner()->EnableInput(PlayerController);

		TArray<UPrimitiveComponent*> Primitives;
		GetOwner()->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			Primitive->OnInputTouchBegin.AddDynamic(this, &UUxtTouchableVolumeComponent::OnInputTouchBeginHandler);
			Primitive->OnInputTouchEnd.AddDynamic(this, &UUxtTouchableVolumeComponent::OnInputTouchEndHandler);
			Primitive->OnInputTouchLeave.AddDynamic(this, &UUxtTouchableVolumeComponent::OnInputTouchLeaveHandler);
		}
	}
}

bool UUxtTouchableVolumeComponent::GetClosestPoint_Implementation(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	float NotUsed;
	if (FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed))
	{
		if (OutClosestPoint == Point)
		{
			OutNormal = OutClosestPoint - Primitive->GetComponentLocation();
		}
		else
		{
			OutNormal = OutClosestPoint - Point;
		}

		OutNormal.Normalize();
		return true;
	}
	return false;
}

void UUxtTouchableVolumeComponent::OnInputTouchBeginHandler(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	check(TouchedComponent->GetOwner() == GetOwner());
	bool bIsAlreadyInSet;
	ActiveTouches.Add(FingerIndex, &bIsAlreadyInSet);
	check(!bIsAlreadyInSet);

	OnEnterFocus(nullptr);
}

void UUxtTouchableVolumeComponent::OnInputTouchEndHandler(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	check(TouchedComponent->GetOwner() == GetOwner());
	int NumRemoved = ActiveTouches.Remove(FingerIndex);
	check(NumRemoved == 0 || NumRemoved == 1);

	if (NumRemoved)
	{
		OnExitFocus(nullptr);
	}
}

void UUxtTouchableVolumeComponent::OnInputTouchLeaveHandler(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent)
{
	check(TouchedComponent->GetOwner() == GetOwner());
	int NumRemoved = ActiveTouches.Remove(FingerIndex);
	check(NumRemoved == 0 || NumRemoved == 1);

	if (NumRemoved)
	{
		OnExitFocus(nullptr);
	}
}

bool UUxtTouchableVolumeComponent::IsContacted() const
{
	return PokePointers.Num() > 0;
}

bool UUxtTouchableVolumeComponent::IsFocused() const
{
	return NumPointersFocusing > 0;
}

void UUxtTouchableVolumeComponent::OnEnterFocus(UObject* Pointer)
{
	const bool bWasFocused = ++NumPointersFocusing > 1;
	OnBeginFocus.Broadcast(this, Pointer, bWasFocused);
}

void UUxtTouchableVolumeComponent::OnExitFocus(UObject* Pointer)
{
	--NumPointersFocusing;
	const bool bIsFocused = IsFocused();

	OnEndFocus.Broadcast(this, Pointer, bIsFocused);
}

bool UUxtTouchableVolumeComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	if (bIsDisabled)
	{
		return false;
	}

	if (TouchablePrimitives.Num() > 0)
	{
		return TouchablePrimitives.Contains(const_cast<UPrimitiveComponent*>(Primitive));
	}

	return true;
}

bool UUxtTouchableVolumeComponent::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	if (bIsDisabled)
	{
		return false;
	}

	if (TouchablePrimitives.Num() > 0)
	{
		return TouchablePrimitives.Contains(const_cast<UPrimitiveComponent*>(Primitive));
	}

	return true;
}

void UUxtTouchableVolumeComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtTouchableVolumeComponent::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

void UUxtTouchableVolumeComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtTouchableVolumeComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!bIsDisabled)
	{
		// Lock the poking pointer so we remain the focused target as it moves.
		Pointer->SetFocusLocked(true);

		PokePointers.Add(Pointer);
		OnBeginPoke.Broadcast(this, Pointer);
	}
}

void UUxtTouchableVolumeComponent::OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!bIsDisabled)
	{
		OnUpdatePoke.Broadcast(this, Pointer);
	}
}

void UUxtTouchableVolumeComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	// Unlock the pointer focus so that another target can be selected.
	Pointer->SetFocusLocked(false);

	PokePointers.Remove(Pointer);

	if (!bIsDisabled)
	{
		OnEndPoke.Broadcast(this, Pointer);
	}
}

EUxtPokeBehaviour UUxtTouchableVolumeComponent::GetPokeBehaviour_Implementation() const
{
	return EUxtPokeBehaviour::Volume;
}

bool UUxtTouchableVolumeComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	if (bIsDisabled)
	{
		return false;
	}

	if (TouchablePrimitives.Num() > 0)
	{
		return TouchablePrimitives.Contains(const_cast<UPrimitiveComponent*>(Primitive));
	}

	return true;
}

bool UUxtTouchableVolumeComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	if (bIsDisabled)
	{
		return false;
	}

	if (TouchablePrimitives.Num() > 0)
	{
		return TouchablePrimitives.Contains(const_cast<UPrimitiveComponent*>(Primitive));
	}

	return true;
}

void UUxtTouchableVolumeComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnEnterFocus(Pointer);
}

void UUxtTouchableVolumeComponent::OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnUpdateFocus.Broadcast(this, Pointer);
}

void UUxtTouchableVolumeComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	OnExitFocus(Pointer);
}

void UUxtTouchableVolumeComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!FarPointerWeak.IsValid() && !bIsDisabled)
	{
		FarPointerWeak = Pointer;
		Pointer->SetFocusLocked(true);
	}
}

void UUxtTouchableVolumeComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get();
	if (Pointer == FarPointer)
	{
		FarPointerWeak = nullptr;
		Pointer->SetFocusLocked(false);
	}
}
