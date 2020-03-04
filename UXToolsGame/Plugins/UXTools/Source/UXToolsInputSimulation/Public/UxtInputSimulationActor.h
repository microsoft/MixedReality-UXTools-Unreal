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
	UFUNCTION(BlueprintPure, Category = "Windows Mixed Reality")
	FName GetTargetPose(EControllerHand Hand) const;

	/** Set the target animation pose for all controlled hands. */
	UFUNCTION(BlueprintCallable, Category = "Windows Mixed Reality")
	void PushTargetPose(FName Name);

	/** Remove the current target animation pose for all controlled hands.
	 *  If another target pose is on the stack it will become the current target pose,
	 *  otherwise hand animation returns to the default pose.
	 */
	UFUNCTION(BlueprintCallable, Category = "Windows Mixed Reality")
	void PopTargetPose(FName Name);

	UFUNCTION(BlueprintGetter)
	UUxtInputSimulationHeadMovementComponent* GetHeadMovement() const { return HeadMovement; }

	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetLeftHand() const { return LeftHand; }

	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetRightHand() const { return RightHand; }

private:

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

	/** Create actor components for HMD simulation. */
	void SetupHeadComponents();
	/** Create actor components for hand simulation. */
	void SetupHandComponents();
	/** Returns the skeletal mesh for the given hand. */
	USkeletalMeshComponent* GetHandMesh(EControllerHand Hand) const;

	/** Copy results of hand animation into the hand state. */
	void CopySimulatedHandState(EControllerHand Hand, FWindowsMixedRealityInputSimulationHandState& HandState) const;

public:

	/** If true, adds default input bindings for input simulation. */
	UPROPERTY(Category = InputSimulation, EditAnywhere, BlueprintReadOnly)
	uint32 bAddDefaultInputBindings : 1;

private:

	/** Movement component for interpreting user input as head movement. */
	UPROPERTY(Category = UxTools, VisibleAnywhere, BlueprintGetter = GetHeadMovement)
	UUxtInputSimulationHeadMovementComponent* HeadMovement;

	/** Skeletal mesh component for the left hand. */
	UPROPERTY(Category = UxTools, VisibleAnywhere, BlueprintGetter = GetLeftHand)
	USkeletalMeshComponent* LeftHand;

	/** Skeletal mesh component for the right hand. */
	UPROPERTY(Category = UxTools, VisibleAnywhere, BlueprintGetter = GetRightHand)
	USkeletalMeshComponent* RightHand;

	/** Set of hands that are actively controlled by user input. */
	TSet<EControllerHand> ControlledHands;

	/** Stack of poses that have been activated by the user. */
	TArray<FName> TargetPoseStack;

};
