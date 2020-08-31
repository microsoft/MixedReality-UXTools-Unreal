// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UxtInputSimulationState.generated.h"

/**
 * Simulation state for a single hand.
 */
struct FUxtInputSimulationHandState
{
	/** True if the hand is currently visible, i.e. simulated as tracked. */
	bool bIsVisible = true;

	/** True if the hand is currently controlled by the user. */
	bool bIsControlled = false;

	/** Transform offset relative to the rest pose. */
	FTransform RelativeTransform = FTransform();

	/** Target pose. */
	FName TargetPose = NAME_None;
};

/**
 * Simulation state for head movement and hand gestures.
 */
UCLASS(BlueprintType)
class UXTOOLSINPUTSIMULATION_API UUxtInputSimulationState : public UObject
{
	GENERATED_BODY()

public:

	UUxtInputSimulationState();

	/** Reset to default. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void Reset();

	/** True if the hand is currently visible. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	bool IsHandVisible(EControllerHand Hand) const;

	/** Set the hand visibility. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void SetHandVisibility(EControllerHand Hand, bool bIsVisible);

	/** True if the hand is currently controlled by the user. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	bool IsHandControlled(EControllerHand Hand) const;

	/** True if any hand is currently controlled by the user. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	bool IsAnyHandControlled() const;

	/** Find all hands that are currently controlled. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	TArray<EControllerHand> GetControlledHands() const;

	/** Enable control of a simulated hand by the user.
	 *  Returns true if hand control was successfully changed.
	 */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	bool SetHandControlEnabled(EControllerHand Hand, bool bEnabled);

	/** Get the current target transform for a hand.
	 *  If bAnimate is true then the transform should be blended over time,
	 *  otherwise the target transform should be applied immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void GetTargetHandTransform(EControllerHand Hand, FTransform& TargetTransform, bool& bAnimate) const;

	/** Add hand movement input along a local axis to all controlled hands. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void AddHandMovementInput(EAxis::Type TranslationAxis, float Value);

	/** Add hand rotation input about a local axis to all controlled hands. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void AddHandRotationInput(EAxis::Type RotationAxis, float Value);

	/** Set the mesh for the given hand to the default location. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void SetDefaultHandLocation(EControllerHand Hand);

	/** Set the rotation for the given hand to the rest rotation. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void SetDefaultHandRotation(EControllerHand Hand);

	/** Get the current animation pose of a hand.
	 *  If the hand is currently controlled by user input it will use the current target pose,
	 *  otherwise the default pose is used.
	 */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	FName GetTargetPose(EControllerHand Hand) const;

	/** Set the target animation pose for a hand. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void SetTargetPose(EControllerHand Hand, FName PoseName);

	/** Reset the default target animation pose for a hand. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void ResetTargetPose(EControllerHand Hand);

	/** Toggle the target pose for all currently controlled hands.
	 *  - If all hands use the target pose already, all hands will reset to the default pose.
	 *  - If any hand does NOT use the target pose already, all hands will use it.
	 */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void TogglePoseForControlledHands(FName PoseName);

private:

	/** Current target pose for each hand. */
	TMap<EControllerHand, FUxtInputSimulationHandState> HandStates;

};
