// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtInputSimulationState.h"

#include "GameFramework/Actor.h"

#include "UxtInputSimulationActor.generated.h"

struct FWindowsMixedRealityInputSimulationHandState;
class UUxtInputSimulationHeadMovementComponent;

/** Actor that produces head pose and hand animations for the input simulation subsystem. */
UCLASS(ClassGroup = "UXTools")
class UXTOOLSINPUTSIMULATION_API AUxtInputSimulationActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintGetter, Category = "Uxt Input Simulation")
	UUxtInputSimulationHeadMovementComponent* GetHeadMovement() const { return HeadMovement; }

	UFUNCTION(BlueprintGetter, Category = "Uxt Input Simulation")
	USkeletalMeshComponent* GetLeftHand() const { return LeftHand; }

	UFUNCTION(BlueprintGetter, Category = "Uxt Input Simulation")
	USkeletalMeshComponent* GetRightHand() const { return RightHand; }

private:
	/** Bind input events to handler functions. */
	void BindInputEvents();

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
	/** Add head rotation input along a local axis. */
	void AddHeadRotationInputImpl(EAxis::Type Axis, float Value);

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

	/** Update the simulated device data from animation and movement results. */
	void UpdateSimulatedDeviceData() const;

public:
	/** If true, adds default input bindings for input simulation. */
	UPROPERTY(EditAnywhere, Category = "Uxt Input Simulation", BlueprintReadOnly)
	uint32 bAddDefaultInputBindings : 1;

private:
	/** Movement component for interpreting user input as head movement. */
	UPROPERTY(VisibleAnywhere, Category = "Uxt Input Simulation", BlueprintGetter = GetHeadMovement)
	UUxtInputSimulationHeadMovementComponent* HeadMovement;

	/** Skeletal mesh component for the left hand. */
	UPROPERTY(VisibleAnywhere, Category = "Uxt Input Simulation", BlueprintGetter = GetLeftHand)
	USkeletalMeshComponent* LeftHand;

	/** Skeletal mesh component for the right hand. */
	UPROPERTY(VisibleAnywhere, Category = "Uxt Input Simulation", BlueprintGetter = GetRightHand)
	USkeletalMeshComponent* RightHand;

	/** Persistent simulation state, cached for quick runtime access. */
	TWeakObjectPtr<UUxtInputSimulationState> SimulationStateWeak;
};
