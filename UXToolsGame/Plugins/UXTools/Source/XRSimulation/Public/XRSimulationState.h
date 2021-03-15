// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

struct XRSIMULATION_API FXRInputAnimationUtils
{
	static const float InputYawScale;
	static const float InputPitchScale;
	static const float InputRollScale;

	/** Select rotation axis for head or hand rotation modes. */
	static EAxis::Type GetInputRotationAxis(EAxis::Type MoveAxis);

	/** Scale hand rotation input value. */
	static float GetHandRotationInputValue(EAxis::Type RotationAxis, float MoveValue);

	/** Scale head rotation input value. */
	static float GetHeadRotationInputValue(EAxis::Type RotationAxis, float MoveValue);
};

/**
 * Simulation state for a single hand.
 */
struct XRSIMULATION_API FXRSimulationHandState
{
	/** True if the hand is currently visible, i.e. simulated as tracked. */
	bool bIsVisible = true;

	/** True if the hand is currently controlled by the user. */
	bool bIsControlled = false;

	/** Transform offset relative to the rest pose. */
	FTransform RelativeTransform = FTransform();

	/** Target pose. */
	FName TargetPose = NAME_None;

	/**
	 * Relative transform of palm to wrist, frozen at the time grip starts.
	 * This is used to produce a stable grip pose by ignoring palm animation during grab.
	 */
	TOptional<FTransform> GripToWristTransform;
};

enum class XRSIMULATION_API EXRSimulationHandMode : uint8
{
	/** Move hands when adding input */
	Movement,
	/** Rotate hands when adding input */
	Rotation,
};

/**
 * Simulation state for head movement and hand gestures.
 */
struct XRSIMULATION_API FXRSimulationState
{
public:
	FXRSimulationState();

	/** Reset to default. */
	void Reset();

	/** Reset hand state to default. */
	void ResetHandState(EControllerHand Hand);

	/** True if the hand is currently visible. */
	bool IsHandVisible(EControllerHand Hand) const;

	/** Set the hand visibility. */
	void SetHandVisibility(EControllerHand Hand, bool bIsVisible);

	/** True if the hand is currently controlled by the user. */
	bool IsHandControlled(EControllerHand Hand) const;

	/** True if any hand is currently controlled by the user. */
	bool IsAnyHandControlled() const;

	/** Find all hands that are currently controlled. */
	TArray<EControllerHand> GetControlledHands() const;

	/** Enable control of a simulated hand by the user.
	 *  Returns true if hand control was successfully changed.
	 */
	bool SetHandControlEnabled(EControllerHand Hand, bool bEnabled);

	/** Get the current target transform for a hand.
	 *  If bAnimate is true then the transform should be blended over time,
	 *  otherwise the target transform should be applied immediately.
	 */
	void GetTargetHandTransform(EControllerHand Hand, FTransform& TargetTransform, bool& bAnimate) const;

	/** Add hand movement or rotation, depending on hand input mode. */
	void AddHandInput(EAxis::Type Axis, float Value);

	/** Add hand movement input along a local axis to all controlled hands. */
	void AddHandMovementInput(EAxis::Type TranslationAxis, float Value);

	/** Add hand rotation input about a local axis to all controlled hands. */
	void AddHandRotationInput(EAxis::Type RotationAxis, float Value);

	/** Set the mesh for the given hand to the default location. */
	void SetDefaultHandLocation(EControllerHand Hand);

	/** Set the rotation for the given hand to the rest rotation. */
	void SetDefaultHandRotation(EControllerHand Hand);

	/** Get the current animation pose of a hand.
	 *  If the hand is currently controlled by user input it will use the current target pose,
	 *  otherwise the default pose is used.
	 */
	FName GetTargetPose(EControllerHand Hand) const;

	/** Set the target animation pose for a hand. */
	void SetTargetPose(EControllerHand Hand, FName PoseName);

	/** Reset the default target animation pose for a hand. */
	void ResetTargetPose(EControllerHand Hand);

	/** Toggle the target pose for all currently controlled hands.
	 *  - If all hands use the target pose already, all hands will reset to the default pose.
	 *  - If any hand does NOT use the target pose already, all hands will use it.
	 */
	void TogglePoseForControlledHands(FName PoseName);

	/** True if grip transform offset has been set. */
	bool HasGripToWristTransform(EControllerHand Hand) const;

	/** Get the frozen grip transform offset from the wrist.
	 * @return Valid if the grip transform offset has been set.
	 */
	FTransform GetGripToWristTransform(EControllerHand Hand) const;

	/** Set the frozen grip transform offset from the wrist. */
	void SetGripToWristTransform(EControllerHand Hand, const FTransform& GripToWristTransform);

	/** Clear the frozen grip transform offset from the wrist. */
	void ClearGripToWristTransform(EControllerHand Hand);

public:
	/** If true, input will be interpreted as hand rotation instead of movement. */
	EXRSimulationHandMode HandInputMode = EXRSimulationHandMode::Movement;

	/** Head position relative to the character controller. */
	FVector RelativeHeadPosition = FVector::ZeroVector;

	/** Head orientation relative to the character controller. */
	FQuat RelativeHeadOrientation = FQuat::Identity;

private:
	/** Current target pose for each hand. */
	TMap<EControllerHand, FXRSimulationHandState> HandStates;
};
