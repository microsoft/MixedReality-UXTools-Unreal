// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationState.h"

#include "UxtRuntimeSettings.h"

#define LOCTEXT_NAMESPACE "UXToolsInputSimulation"

const float FUxtInputAnimationUtils::InputYawScale = 2.5f;
const float FUxtInputAnimationUtils::InputPitchScale = 1.75f;
const float FUxtInputAnimationUtils::InputRollScale = 5.0f;

/** Select rotation axis for head or hand rotation modes. */
EAxis::Type FUxtInputAnimationUtils::GetInputRotationAxis(EAxis::Type MoveAxis)
{
	switch (MoveAxis)
	{
	// Roll with forward axis
	case EAxis::X:
		return EAxis::X;
		// Yaw with right axis
	case EAxis::Y:
		return EAxis::Z;
		// Pitch with up axis
	case EAxis::Z:
		return EAxis::Y;
	}
	return EAxis::None;
}

/** Scale hand rotation input value. */
float FUxtInputAnimationUtils::GetHandRotationInputValue(EAxis::Type RotationAxis, float MoveValue)
{
	switch (RotationAxis)
	{
	case EAxis::X:
		return MoveValue * InputRollScale;
	case EAxis::Y:
		return MoveValue * InputPitchScale;
	case EAxis::Z:
		return MoveValue * InputYawScale;
	}
	return 0.0f;
}

/** Scale head rotation input value. */
float FUxtInputAnimationUtils::GetHeadRotationInputValue(EAxis::Type RotationAxis, float MoveValue)
{
	switch (RotationAxis)
	{
	case EAxis::X:
		return 0.0f; // No head roll
	case EAxis::Y:
		return MoveValue * InputPitchScale;
	case EAxis::Z:
		return MoveValue * InputYawScale;
	}
	return 0.0f;
}

UUxtInputSimulationState::UUxtInputSimulationState()
{
	Reset();
}

void UUxtInputSimulationState::Reset()
{
	RelativeHeadPosition = FVector::ZeroVector;
	RelativeHeadOrientation = FQuat::Identity;

	ResetHandState(EControllerHand::Left);
	ResetHandState(EControllerHand::Right);
}

bool UUxtInputSimulationState::IsHandVisible(EControllerHand Hand) const
{
	return HandStates.FindRef(Hand).bIsVisible;
}

void UUxtInputSimulationState::SetHandVisibility(EControllerHand Hand, bool bIsVisible)
{
	if (bIsVisible)
	{
		// Reset hand position when it becomes visible
		if (!IsHandVisible(Hand))
		{
			ResetHandState(Hand);
		}
	}
	else
	{
		// Untracked hands can not be controlled.
		SetHandControlEnabled(Hand, false);
	}

	HandStates.FindOrAdd(Hand).bIsVisible = bIsVisible;
}

bool UUxtInputSimulationState::IsHandControlled(EControllerHand Hand) const
{
	return HandStates.FindRef(Hand).bIsControlled;
}

bool UUxtInputSimulationState::IsAnyHandControlled() const
{
	for (const auto& KeyValuePair : HandStates)
	{
		if (KeyValuePair.Value.bIsControlled)
		{
			return true;
		}
	}
	return false;
}

TArray<EControllerHand> UUxtInputSimulationState::GetControlledHands() const
{
	TArray<EControllerHand> Keys;
	HandStates.GetKeys(Keys);
	return Keys.FilterByPredicate([this](EControllerHand Hand) -> bool { return HandStates[Hand].bIsControlled; });
}

bool UUxtInputSimulationState::SetHandControlEnabled(EControllerHand Hand, bool bEnabled)
{
	if (bEnabled)
	{
		// Only allow control when the hand is visible.
		if (!IsHandVisible(Hand))
		{
			return false;
		}
	}

	HandStates.FindOrAdd(Hand).bIsControlled = bEnabled;
	return true;
}

void UUxtInputSimulationState::GetTargetHandTransform(EControllerHand Hand, FTransform& TargetTransform, bool& bAnimate) const
{
	const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	// Mirror the left hand.
	FVector Scale3D = (Hand == EControllerHand::Left ? FVector(1, -1, 1) : FVector(1, 1, 1));

	const FTransform& HandTransform = HandStates.FindRef(Hand).RelativeTransform;
	if (GetTargetPose(Hand) == Settings->MenuHandPose)
	{
		// Use a camera-facing rotation for the menu pose
		FRotator UserFacing = (Hand == EControllerHand::Left ? FRotator(30, 150, -20) : FRotator(30, 210, 20));
		TargetTransform = FTransform(UserFacing, HandTransform.GetLocation(), Scale3D);
		// Blend in and out of the menu pose
		bAnimate = true;
	}
	else
	{
		FRotator RestRotation = Settings->HandRestOrientation;
		if (Hand == EControllerHand::Left)
		{
			RestRotation.Yaw = -RestRotation.Yaw;
			RestRotation.Roll = -RestRotation.Roll;
		}

		TargetTransform =
			FTransform(HandTransform.GetRotation() * RestRotation.Quaternion().Inverse(), HandTransform.GetLocation(), Scale3D);
		// No animation between user rotations
		bAnimate = false;
	}
}

void UUxtInputSimulationState::AddHandInput(EAxis::Type Axis, float Value)
{
	EAxis::Type RotationAxis = FUxtInputAnimationUtils::GetInputRotationAxis(Axis);
	float RotationValue = FUxtInputAnimationUtils::GetHandRotationInputValue(RotationAxis, Value);

	switch (HandInputMode)
	{
	case EUxtInputSimulationHandMode::Movement:
		AddHandMovementInput(Axis, Value);
		break;
	case EUxtInputSimulationHandMode::Rotation:
		AddHandRotationInput(RotationAxis, RotationValue);
		break;
	}
}

void UUxtInputSimulationState::AddHandMovementInput(EAxis::Type TranslationAxis, float Value)
{
	if (Value != 0.f)
	{
		FVector Dir = FRotationMatrix::Identity.GetUnitAxis(TranslationAxis);
		for (auto& KeyValuePair : HandStates)
		{
			FUxtInputSimulationHandState& HandState = KeyValuePair.Value;
			if (HandState.bIsControlled)
			{
				HandState.RelativeTransform.AddToTranslation(Dir * Value);
			}
		}
	}
}

void UUxtInputSimulationState::AddHandRotationInput(EAxis::Type RotationAxis, float Value)
{
	if (Value != 0.f)
	{
		for (auto& KeyValuePair : HandStates)
		{
			EControllerHand Hand = KeyValuePair.Key;
			FUxtInputSimulationHandState& HandState = KeyValuePair.Value;
			if (HandState.bIsControlled)
			{
				FRotator DeltaRot = FRotator::ZeroRotator;
				DeltaRot.SetComponentForAxis(RotationAxis, Value);
				// Mirror roll value so hands turn in opposite directions for symmetry.
				if (Hand == EControllerHand::Left)
				{
					DeltaRot.Roll = -DeltaRot.Roll;
				}

				FRotator NewRot = HandState.RelativeTransform.Rotator() + DeltaRot;
				NewRot.Pitch = FMath::ClampAngle(NewRot.Pitch, -90.0f, 90.0f);
				HandState.RelativeTransform.SetRotation(NewRot.Quaternion());
			}
		}
	}
}

void UUxtInputSimulationState::ResetHandState(EControllerHand Hand)
{
	const UUxtRuntimeSettings* Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	SetDefaultHandLocation(Hand);
	SetDefaultHandRotation(Hand);
	ResetTargetPose(Hand);

	// Avoid recursive SetHandVisibility function call by changing state directly
	HandStates.FindOrAdd(Hand).bIsVisible = Settings->bStartWithHandsEnabled;
}

void UUxtInputSimulationState::SetDefaultHandLocation(EControllerHand Hand)
{
	const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	FUxtInputSimulationHandState& HandState = HandStates.FindOrAdd(Hand);

	FVector DefaultPos = Settings->DefaultHandPosition;
	if (Hand == EControllerHand::Left)
	{
		DefaultPos.Y = -DefaultPos.Y;
	}
	HandState.RelativeTransform.SetLocation(DefaultPos);
}

void UUxtInputSimulationState::SetDefaultHandRotation(EControllerHand Hand)
{
	const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	FUxtInputSimulationHandState& HandState = HandStates.FindOrAdd(Hand);

	FRotator DefaultRot = Settings->HandRestOrientation;
	if (Hand == EControllerHand::Left)
	{
		DefaultRot.Yaw = -DefaultRot.Yaw;
		DefaultRot.Roll = -DefaultRot.Roll;
	}
	HandState.RelativeTransform.SetRotation(DefaultRot.Quaternion());
}

FName UUxtInputSimulationState::GetTargetPose(EControllerHand Hand) const
{
	const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	const FUxtInputSimulationHandState& HandState = HandStates.FindRef(Hand);
	return HandState.TargetPose.IsNone() ? Settings->DefaultHandPose : HandState.TargetPose;
}

void UUxtInputSimulationState::SetTargetPose(EControllerHand Hand, FName PoseName)
{
	HandStates.FindOrAdd(Hand).TargetPose = PoseName;
}

void UUxtInputSimulationState::ResetTargetPose(EControllerHand Hand)
{
	HandStates.FindOrAdd(Hand).TargetPose = NAME_None;
}

void UUxtInputSimulationState::TogglePoseForControlledHands(FName PoseName)
{
	TArray<EControllerHand> ControlledHands = GetControlledHands();

	// Check if all hands are using the pose
	bool bAllHandsUsingPose = true;
	for (EControllerHand Hand : ControlledHands)
	{
		if (GetTargetPose(Hand) != PoseName)
		{
			bAllHandsUsingPose = false;
			break;
		}
	}

	if (bAllHandsUsingPose)
	{
		// All hands currently using the target pose, toggle "off" by resetting to default
		for (EControllerHand Hand : ControlledHands)
		{
			ResetTargetPose(Hand);
		}
	}
	else
	{
		// Not all hands using the target pose, toggle "on" by setting it
		for (EControllerHand Hand : ControlledHands)
		{
			SetTargetPose(Hand, PoseName);
		}
	}
}

#undef LOCTEXT_NAMESPACE
