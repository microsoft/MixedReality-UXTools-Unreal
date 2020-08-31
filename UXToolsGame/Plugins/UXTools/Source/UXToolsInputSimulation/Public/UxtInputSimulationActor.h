// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "UxtInputSimulationState.h"

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

	UFUNCTION(BlueprintGetter)
	UUxtInputSimulationHeadMovementComponent* GetHeadMovement() const { return HeadMovement; }

	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetLeftHand() const { return LeftHand; }

	UFUNCTION(BlueprintGetter)
	USkeletalMeshComponent* GetRightHand() const { return RightHand; }

private:

	void OnToggleLeftHandPressed();
	void OnToggleRightHandPressed();

	void OnControlLeftHandPressed();
	void OnControlLeftHandReleased();
	void OnControlRightHandPressed();
	void OnControlRightHandReleased();

	void OnHandRotatePressed();
	void OnHandRotateReleased();

	void OnPrimaryHandPosePressed();
	void OnSecondaryHandPosePressed();
	void OnMenuHandPosePressed();

	void AddInputMoveForward(float Value);
	void AddInputMoveRight(float Value);
	void AddInputMoveUp(float Value);

	void AddInputLookUp(float Value);
	void AddInputTurn(float Value);
	void AddInputScroll(float Value);

	/** Add head movement input along a local axis. */
	void AddHeadMovementInputImpl(EAxis::Type Axis, float Value);

	/** Set rotation option to interpret look rotation as rotation of hands instead */
	void SetHandRotationEnabled(bool bEnabled);

	/** Create actor components for HMD simulation. */
	void SetupHeadComponents();
	/** Create actor components for hand simulation. */
	void SetupHandComponents();
	/** Returns the skeletal mesh for the given hand. */
	USkeletalMeshComponent* GetHandMesh(EControllerHand Hand) const;
	/** Update hand mesh component based on simulation state */
	void UpdateHandMeshComponent(EControllerHand Hand);

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

	/** If true, the look rotation will be interpreted as hand rotation instead. */
	bool bEnableHandRotation = false;

	/** Persistent simulation state, cached for quick runtime access. */
	TWeakObjectPtr<UUxtInputSimulationState> SimulationStateWeak;

};
