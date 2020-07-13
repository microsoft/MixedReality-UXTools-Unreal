// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTestTargetComponent.h"

#include "Input/UxtNearPointerComponent.h"
#include "UxtTestUtils.h"
#include "UxtTestHandTracker.h"

#include "Components/PrimitiveComponent.h"

void UTestGrabTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
}

void UTestGrabTarget::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestGrabTarget::OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestGrabTarget::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

bool UTestGrabTarget::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return true;
}

void UTestGrabTarget::OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginGrabCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(true);
	}
}

void UTestGrabTarget::OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndGrabCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(false);
	}
}

void UTestPokeTarget::BeginPlay()
{
	Super::BeginPlay();

	BeginFocusCount = 0;
	EndFocusCount = 0;
}

void UTestPokeTarget::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginFocusCount;
}

void UTestPokeTarget::OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
}

void UTestPokeTarget::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndFocusCount;
}

EUxtPokeBehaviour UTestPokeTarget::GetPokeBehaviour() const
{
	return EUxtPokeBehaviour::FrontFace;
}

bool UTestPokeTarget::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive)
{
	return true;
}

void UTestPokeTarget::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	++BeginPokeCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(true);
	}
}

void UTestPokeTarget::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	++EndPokeCount;

	if (bUseFocusLock)
	{
		Pointer->SetFocusLocked(false);
	}
}
