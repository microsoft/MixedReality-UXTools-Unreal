// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "UxtInputSimulationActor.generated.h"

struct FWindowsMixedRealityInputSimulationHandState;
class UUxtInputSimulationHeadMovementComponent;

/** Actor that produces head pose and hand animations for the input simulation subsystem. */
UCLASS(ClassGroup = UXTools)
class UXTOOLSINPUTSIMULATION_API AUxtInputSimulationActor
	: public AActor
{
	GENERATED_UCLASS_BODY()

public:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Get the current animation pose of a hand.
	 *  If the hand is currently controlled by user input it will use the current target pose,
	 *  otherwise the default pose is used.
	 */
	UFUNCTION(BlueprintPure, Category = InputSimulation)
	FName GetTargetPose(EControllerHand Hand) const;

	/** Set the target animation pose for all controlled hands. */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void PushTargetPose(FName Name);

	/** Remove the current target animation pose for all controlled hands.
	 *  If another target pose is on the stack it will become the current target pose,
	 *  otherwise hand animation returns to the default pose.
	 */
	UFUNCTION(BlueprintCallable, Category = InputSimulation)
	void PopTargetPose(FName Name);

	UFUNCTION(BlueprintGetter)
	UUxtInputSimulationHeadMovementComponent* GetHeadMovement() const { return HeadMovement; }

	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetLeftHand() const { return LeftHand; }

	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetRightHand() const { return RightHand; }

	/** True if the hand is currently visible. */
	UFUNCTION(BlueprintPure, Category = InputSimulation)
	bool IsHandVisible(EControllerHand Hand) const;

	/** True if the hand is currently controlled by the user. */
	UFUNCTION(BlueprintPure, Category = InputSimulation)
	bool IsHandControlled(EControllerHand Hand) const;

private:

	void OnToggleLeftHandPressed();
	void OnToggleRightHandPressed();

	void OnControlLeftHandPressed();
	void OnControlLeftHandReleased();
	void OnControlRightHandPressed();
	void OnControlRightHandReleased();

	void OnPrimaryHandPosePressed();
	void OnPrimaryHandPoseReleased();
	void OnSecondaryHandPosePressed();
	void OnSecondaryHandPoseReleased();

	void AddInputMoveForward(float Value);
	void AddInputMoveRight(float Value);
	void AddInputMoveUp(float Value);

	void AddInputLookUp(float Value);
	void AddInputTurn(float Value);
	void AddInputScroll(float Value);

	/** Add head movement input along a local axis. */
	void AddMovementInputImpl(EAxis::Type Axis, float Value);
	/** Add hand movement input along a local axis. */
	void AddHandInputImpl(EAxis::Type Axis, float Value);

	/** Set the mesh for the given hand to the default location. */
	void SetDefaultHandLocation(EControllerHand Hand);

	/** Set the hand visibility. */
	void SetHandVisibility(EControllerHand Hand, bool bIsVisible);

	/** Enable control of a simulated hand by the user.
	 *  Returns true if hand control was successfully changed.
	 */
	bool SetHandControlEnabled(EControllerHand Hand, bool bEnabled);

	/** Create actor components for HMD simulation. */
	void SetupHeadComponents();
	/** Create actor components for hand simulation. */
	void SetupHandComponents();
	/** Returns the skeletal mesh for the given hand. */
	USkeletalMeshComponent* GetHandMesh(EControllerHand Hand) const;

	/** Copy results of hand animation into the hand state. */
	void UpdateSimulatedHandState(EControllerHand Hand, FWindowsMixedRealityInputSimulationHandState& HandState) const;

public:

	/** If true, adds default input bindings for input simulation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = InputSimulation)
	uint32 bAddDefaultInputBindings : 1;

private:

	/** Movement component for interpreting user input as head movement. */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetHeadMovement, Category = InputSimulation)
	UUxtInputSimulationHeadMovementComponent* HeadMovement;

	/** Skeletal mesh component for the left hand. */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetLeftHand, Category = InputSimulation)
	USkeletalMeshComponent* LeftHand;

	/** Skeletal mesh component for the right hand. */
	UPROPERTY(VisibleAnywhere, BlueprintGetter = GetRightHand, Category = InputSimulation)
	USkeletalMeshComponent* RightHand;

	/** Set of hands that are actively controlled by user input. */
	TSet<EControllerHand> ControlledHands;

	/** Stack of poses that have been activated by the user. */
	TArray<FName> TargetPoseStack;

};
