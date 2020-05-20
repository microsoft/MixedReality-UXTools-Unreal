// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "UxtHandConstraintComponent.generated.h"

/** Zone relative to the hand in which the object is placed. */
UENUM(BlueprintType)
enum class EUxtHandConstraintZone : uint8
{
	/** Area opposite the thumb. */
	UlnarSide,
	/** Area on the thumb side. */
	RadialSide,
	/** Above the finger tips. */
	AboveFingerTips,
	/** Below the wrist. */
	BelowWrist,
};

/** Frame of reference for placement. */
UENUM(BlueprintType)
enum class EUxtHandConstraintOffsetMode : uint8
{
	/** Uses the camera view to compute the offset. */
	LookAtCamera,
	/** Uses the hand rotation to compute the offset. */
	HandRotation,
};

/** Goal rotation mode. */
UENUM(BlueprintType)
enum class EUxtHandConstraintRotationMode : uint8
{
	/** Do not change the rotation */
	None,
	/** Rotate towards the camera. */
	LookAtCamera,
	/** Uses the hand rotation. */
	HandRotation,
};

/**
 * Component that calculates a goal based on hand tracking and moves the owning actor.
 * 
 * Several zones around the hand supported: radial and ulnar for the thumb side and its opposite,
 * as well as above and below the hand. The goal position is computed by casting a ray
 * in the direction of the zone at a bounding box around the hand joints.
 *
 * The constraint can be oriented on either the hand rotation alone or facing the player.
 */
UCLASS(ClassGroup = UXTools, meta=(BlueprintSpawnableComponent))
class UXTOOLS_API UUxtHandConstraintComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UUxtHandConstraintComponent();

	UFUNCTION(BlueprintGetter)
	EControllerHand GetTrackedHand() const;

	UFUNCTION(BlueprintGetter)
	const FBox& GetHandBounds() const;

	UFUNCTION(BlueprintGetter)
	bool HasValidGoal() const;

	UFUNCTION(BlueprintGetter)
	const FVector& GetGoalLocation() const;

	UFUNCTION(BlueprintGetter)
	const FQuat& GetGoalRotation() const;
	
public:

	/**
	 * Hand to use for the constraint.
	 * If set to 'Any Hand' the first tracked hand will be used, until tracking is lost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint", meta = (ExposeOnSpawn = true))
	EControllerHand Hand = EControllerHand::AnyHand;

	/** Safe zone that determines the target location of the constraint relative to the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hand Constraint")
	EUxtHandConstraintZone Zone = EUxtHandConstraintZone::UlnarSide;

	/** Determines how the offset vector is computed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint")
	EUxtHandConstraintOffsetMode OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;

	/** Determines how the goal rotation is computed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint")
	EUxtHandConstraintRotationMode RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;

	/** Margin between the hand bounding box and the goal location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint")
	float GoalMargin = 0.0f;

	/**
	 * Actor transform is moved towards the goal if true.
	 * Disable this to only compute the goal without changing the actor transform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint", meta = (ClampMin = "0.0"))
	bool bMoveOwningActor = true;

	/**
	 * Interpolation time for smoothed translation.
	 * Set to zero to disable smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint", meta = (ClampMin = "0.0"))
	float LocationLerpTime = 0.05f;

	/**
	 * Interpolation time for smoothed rotation.
	 * Set to zero to disable smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Constraint", meta = (ClampMin = "0.0"))
	float RotationLerpTime = 0.05f;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:

	/** Direction of the safe zone from the hand origin. */
	FVector GetZoneDirection(const FVector& HandLocation, const FQuat& HandRotation) const;

	/**
	 * Check for available hands and set the current tracked hand.
	 * Returns true if a tracked hand was found and the output location and rotation are valid.
	 */
	bool UpdateHandTracking(FVector& OutPalmLocation, FQuat& OutPalmRotation);

	/** Compute hand bounding box from joint transforms and radii. */
	void UpdateHandBounds(const FVector& PalmLocation, const FQuat& PalmRotation);

	/** Compute goal location and rotation by projecting onto the hand bounds. */
	void UpdateGoal(const FVector& PalmLocation, const FQuat& PalmRotation);

	/** Move the actor towards the target location and rotation. */
	void AddMovement(float DeltaTime);

private:

	/** Actual hand that is currently tracked for the constraint. */
	UPROPERTY(BlueprintGetter = GetTrackedHand, Transient, Category = "Hand Constraint")
	EControllerHand TrackedHand;

	/** Bounding box of hand joints with radii, aligned with the palm bone. */
	UPROPERTY(BlueprintGetter = GetHandBounds, Transient, Category = "Hand Constraint")
	FBox HandBounds;

	/** True if the goal location is valid, otherwise goal should not be used */
	UPROPERTY(BlueprintGetter = HasValidGoal, Transient, Category = "Hand Constraint")
	bool bHasValidGoal;

	/** Goal location for the constraint. */
	UPROPERTY(BlueprintGetter = GetGoalLocation, Transient, Category = "Hand Constraint")
	FVector GoalLocation;

	/** Goal rotation for the constraint. */
	UPROPERTY(BlueprintGetter = GetGoalRotation, Transient, Category = "Hand Constraint")
	FQuat GoalRotation;

	/** Cached enum info for hand joints */
	const UEnum* EnumUxtHandJoint;

};
