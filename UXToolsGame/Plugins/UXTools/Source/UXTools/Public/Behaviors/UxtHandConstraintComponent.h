// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "Components/ActorComponent.h"

#include "UxtHandConstraintComponent.generated.h"

class UUxtHandConstraintComponent;

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUxtHandConstraintActivatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUxtHandConstraintDeactivatedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtHandConstraintBeginTrackingDelegate, EControllerHand, TrackedHand);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtHandConstraintEndTrackingDelegate, EControllerHand, TrackedHand);

/**
 * Component that calculates a goal based on hand tracking and moves the owning actor.
 *
 * Several zones around the hand supported: radial and ulnar for the thumb side and its opposite,
 * as well as above and below the hand. The goal position is computed by casting a ray
 * in the direction of the zone at a bounding box around the hand joints.
 *
 * The constraint can be oriented on either the hand rotation alone or facing the player.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtHandConstraintComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUxtHandConstraintComponent();

	UFUNCTION(BlueprintGetter, Category = "Uxt Hand Constraint")
	EControllerHand GetTrackedHand() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Hand Constraint")
	const FBox& GetHandBounds() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Hand Constraint")
	bool IsConstraintActive() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Hand Constraint")
	const FVector& GetGoalLocation() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Hand Constraint")
	const FQuat& GetGoalRotation() const;

	/**
	 * Returns true if the given hand is eligible for the constraint.
	 * If the hand is rejected the constraint will be deactivated.
	 */
	UFUNCTION(BlueprintPure, Category = "Uxt Hand Constraint")
	virtual bool IsHandUsableForConstraint(EControllerHand NewHand);

public:
	/**
	 * Hand to use for the constraint.
	 * If set to 'Any Hand' the first tracked hand will be used, until tracking is lost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint", meta = (ExposeOnSpawn = true))
	EControllerHand Hand = EControllerHand::AnyHand;

	/** Safe zone that determines the target location of the constraint relative to the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint")
	EUxtHandConstraintZone Zone = EUxtHandConstraintZone::UlnarSide;

	/** Determines how the offset vector is computed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint")
	EUxtHandConstraintOffsetMode OffsetMode = EUxtHandConstraintOffsetMode::LookAtCamera;

	/** Determines how the goal rotation is computed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint")
	EUxtHandConstraintRotationMode RotationMode = EUxtHandConstraintRotationMode::LookAtCamera;

	/** Margin between the hand bounding box and the goal location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint")
	float GoalMargin = 0.0f;

	/**
	 * Actor transform is moved towards the goal if true.
	 * Disable this to only compute the goal without changing the actor transform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint")
	bool bMoveOwningActor = true;

	/**
	 * Interpolation time for smoothed translation.
	 * Set to zero to disable smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint", meta = (ClampMin = "0.0"))
	float LocationLerpTime = 0.05f;

	/**
	 * Interpolation time for smoothed rotation.
	 * Set to zero to disable smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Hand Constraint", meta = (ClampMin = "0.0"))
	float RotationLerpTime = 0.05f;

	/** Event raised when the constraint becomes active, as indicated by the bIsConstraintActive property. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Hand Constraint")
	FUxtHandConstraintActivatedDelegate OnConstraintActivated;

	/** Event raised when the constraint becomes inactive, as indicated by the bIsConstraintActive property. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Hand Constraint")
	FUxtHandConstraintDeactivatedDelegate OnConstraintDeactivated;

	/** Event raised when the constraint begins tracking a hand. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Hand Constraint")
	FUxtHandConstraintBeginTrackingDelegate OnBeginTracking;

	/** Event raised when the constraint ends tracking a hand. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Hand Constraint")
	FUxtHandConstraintEndTrackingDelegate OnEndTracking;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Direction of the safe zone from the hand origin. */
	FVector GetZoneDirection(const FVector& HandLocation, const FQuat& HandRotation) const;

	/** Updates hand tracking state, hand bounds, goal positions, and determines if the constraint is active. */
	void UpdateConstraint();

	/**
	 * Check for available hands and set the current tracked hand.
	 * Returns true if a tracked hand was found and the output location and rotation are valid.
	 */
	bool UpdateTrackedHand(FVector& OutPalmLocation, FQuat& OutPalmRotation);

	/**
	 * Compute hand bounding box from joint transforms and radii.
	 * Returns true if the hand bounds were successfully updated.
	 */
	bool UpdateHandBounds(const FVector& PalmLocation, const FQuat& PalmRotation);

	/**
	 * Compute goal location and rotation by projecting onto the hand bounds.
	 * Returns true if a valid goal could be computed.
	 */
	bool UpdateGoal(const FVector& PalmLocation, const FQuat& PalmRotation);

	/** Move the actor towards the target location and rotation. */
	void AddMovement(float DeltaTime);

private:
	/** Actual hand that is currently tracked for the constraint. */
	UPROPERTY(Transient, Category = "Uxt Hand Constraint", BlueprintGetter = GetTrackedHand)
	EControllerHand TrackedHand;

	/** Bounding box of hand joints with radii, aligned with the palm bone. */
	UPROPERTY(Transient, Category = "Uxt Hand Constraint", BlueprintGetter = GetHandBounds)
	FBox HandBounds;

	/**
	 * True if a usable hand was found and the constraint goal is valid.
	 * OnConstraintActivated and OnConstraintDeactivated will be called when the active state changes.
	 * While the constraint is active and the bMoveOwningActor flag is set it will move the actor towards the goal.
	 */
	UPROPERTY(Transient, Category = "Uxt Hand Constraint", BlueprintGetter = IsConstraintActive)
	bool bIsConstraintActive;

	/** Goal location for the constraint. */
	UPROPERTY(Transient, Category = "Uxt Hand Constraint", BlueprintGetter = GetGoalLocation)
	FVector GoalLocation;

	/** Goal rotation for the constraint. */
	UPROPERTY(Transient, Category = "Uxt Hand Constraint", BlueprintGetter = GetGoalRotation)
	FQuat GoalRotation;
};
