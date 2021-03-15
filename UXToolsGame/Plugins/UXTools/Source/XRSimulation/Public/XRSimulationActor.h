// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayTypes.h"
#include "XRSimulationState.h"

#include "GameFramework/Actor.h"

#include "XRSimulationActor.generated.h"

struct FXRMotionControllerData;
class UXRSimulationHeadMovementComponent;

/** Actor that produces head pose and hand animations for the simulated HMD. */
UCLASS(ClassGroup = "XRSimulation")
class XRSIMULATION_API AXRSimulationActor : public AActor
{
	GENERATED_UCLASS_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetSimulationState(const TSharedPtr<FXRSimulationState>& NewSimulationState);
	void SetTrackingToWorldTransform(const FTransform& InTrackingToWorldTransform);

	/* Returns the target hand pose from the simulation state for animation. */
	UFUNCTION(BlueprintPure, Category = "XRSimulation", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	void GetTargetHandPose(
		UObject* WorldContextObject, EControllerHand Hand, FName& TargetPose, FTransform& TargetTransform, bool& bAnimateTransform) const;

	void GetHeadPose(FQuat& Orientation, FVector& Position) const;

	void GetHandData(EControllerHand Hand, FXRMotionControllerData& MotionControllerData) const;

	void GetControllerActionState(EControllerHand Hand, bool& OutSelectPressed, bool& OutGripPressed) const;

	UFUNCTION(BlueprintGetter, Category = "XRSimulation")
	UXRSimulationHeadMovementComponent* GetHeadMovement() const { return HeadMovement; }

	UFUNCTION(BlueprintGetter, Category = "XRSimulation")
	USkeletalMeshComponent* GetLeftHand() const { return LeftHand; }

	UFUNCTION(BlueprintGetter, Category = "XRSimulation")
	USkeletalMeshComponent* GetRightHand() const { return RightHand; }

	static void RegisterInputMappings();
	static void UnregisterInputMappings();

private:
	/** Find bone transforms matching the requested keypoints in the skeletal hand mesh. */
	bool GetKeypointTransforms(
		EControllerHand Hand, const TArray<EHandKeypoint>& Keypoints, TArray<FTransform>& OutTransforms, TArray<float>& OutRadii) const;

	/** Set or clear the GripToWristTransform when grip starts or stops. */
	void UpdateStabilizedGripTransform(EControllerHand Hand);

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

public:
	/** If true, adds default input bindings for input simulation. */
	UPROPERTY(EditAnywhere, Category = "XRSimulation", BlueprintReadOnly)
	uint32 bAddDefaultInputBindings : 1;

private:
	/** Movement component for interpreting user input as head movement. */
	UPROPERTY(VisibleAnywhere, Category = "XRSimulation", BlueprintGetter = GetHeadMovement)
	UXRSimulationHeadMovementComponent* HeadMovement;

	/** Skeletal mesh component for the left hand. */
	UPROPERTY(VisibleAnywhere, Category = "XRSimulation", BlueprintGetter = GetLeftHand)
	USkeletalMeshComponent* LeftHand;

	/** Skeletal mesh component for the right hand. */
	UPROPERTY(VisibleAnywhere, Category = "XRSimulation", BlueprintGetter = GetRightHand)
	USkeletalMeshComponent* RightHand;

	/** Persistent simulation state, cached for quick runtime access. */
	TSharedPtr<FXRSimulationState> SimulationState;

	/** Tracking-to-World transform of the HMD owning this actor.
	 * This transform is applied in parent space to the hand component transforms.
	 */
	FTransform TrackingToWorldTransform = FTransform::Identity;
};
