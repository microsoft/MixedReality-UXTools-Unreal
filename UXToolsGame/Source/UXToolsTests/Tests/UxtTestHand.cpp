// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTestHand.h"

#include "Engine.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

FUxtTestHand::FUxtTestHand(EControllerHand TargetHand)
{
	Hand = TargetHand;
}

void FUxtTestHand::Configure(EUxtInteractionMode TargetInteractionMode, const FVector& TargetLocation)
{
	check(TargetInteractionMode != EUxtInteractionMode::None);

	InteractionMode = TargetInteractionMode;

	switch (InteractionMode)
	{
		case EUxtInteractionMode::Near:
			Transform.SetLocation(TargetLocation + FVector(-5, 0, 0));
			Pointer = UxtTestUtils::CreateNearPointer(UxtTestUtils::GetTestWorld(), "Near Pointer", Transform.GetLocation(), Hand);
			break;

		case EUxtInteractionMode::Far:
			Transform.SetLocation(TargetLocation + FVector(-200, 0, 0));
			Pointer = UxtTestUtils::CreateFarPointer(UxtTestUtils::GetTestWorld(), "Far Pointer", Transform.GetLocation(), Hand);
			break;

		default:
			checkNoEntry();
			break;
	}
}

void FUxtTestHand::Reset()
{
	InteractionMode = EUxtInteractionMode::None;
	Transform = FTransform::Identity;

	if (Pointer)
	{
		Pointer->GetOwner()->Destroy();
		Pointer = nullptr;
	}
}

void FUxtTestHand::Translate(const FVector& Translation)
{
	check(InteractionMode != EUxtInteractionMode::None && "Test hand must be configured before use");

	SetTranslation(Transform.GetTranslation() + Translation);
}

void FUxtTestHand::Rotate(const FQuat& Rotation)
{
	check(InteractionMode != EUxtInteractionMode::None && "Test hand must be configured before use");

	SetRotation(Transform.GetRotation() * Rotation);
}

void FUxtTestHand::SetTranslation(const FVector& Translation)
{
	check(InteractionMode != EUxtInteractionMode::None && "Test hand must be configured before use");

	Transform.SetLocation(Translation);

	UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Transform.GetTranslation(), Hand);
}

void FUxtTestHand::SetRotation(const FQuat& Rotation)
{
	check(InteractionMode != EUxtInteractionMode::None && "Test hand must be configured before use");

	Transform.SetRotation(Rotation);

	UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(Transform.GetRotation(), Hand);
}

void FUxtTestHand::SetGrabbing(bool bIsGrabbing)
{
	check(InteractionMode != EUxtInteractionMode::None && "Test hand must be configured before use");

	switch (InteractionMode)
	{
		case EUxtInteractionMode::Near:
			UxtTestUtils::GetTestHandTracker().SetGrabbing(bIsGrabbing, Hand);
			break;

		case EUxtInteractionMode::Far:
			UxtTestUtils::GetTestHandTracker().SetSelectPressed(bIsGrabbing, Hand);
			break;

		default:
			checkNoEntry();
			break;
	}
}
