// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Behaviors/UxtHandConstraintComponent.h"

#include "UxtPalmUpConstraintComponent.generated.h"

/**
 * Hand constraint component that becomes active if the hand is facing the player camera.
 *
 * The palm must be facing the camera for the constraint to be active.
 * Optionally the hand can also be rejected if it isn't flat.
 */
UCLASS(Blueprintable, ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtPalmUpConstraintComponent : public UUxtHandConstraintComponent
{
	GENERATED_BODY()

public:
	virtual bool IsHandUsableForConstraint(EControllerHand NewHand) override;

public:
	/**
	 * Maximum allowed angle between the negative palm normal and view vector.
	 * If the angle exceeds the limit the hand is not used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Palm Up Constraint", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPalmAngle = 75.0f;

	/**
	 * If true then the hand needs to be flat to be accepted.
	 * The triangle between index, ring finger, and palm needs to be aligned with the palm within MaxFlatHandAngle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Palm Up Constraint")
	bool bRequireFlatHand = false;

	/** Maximum allowed angle between palm and index/ring finger/palm triangle to be considered a flat hand. */
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Uxt Palm Up Constraint",
		meta = (EditCondition = "bRequireFlatHand", ClampMin = "0.0", ClampMax = "90.0"))
	float MaxFlatHandAngle = 45.0f;

	/**
	 * If true then the user must be looking at their hand to be accepted.
	 * Head gaze will be used if an eye tracker is not available.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Palm Up Constraint")
	bool bRequireGaze = false;

	/**
	 * The maximum distance between the eye gaze location on the hand plane and the reference point to accept the gaze.
	 */
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Uxt Palm Up Constraint", meta = (EditCondition = "bRequireGaze", ClampMin = "0.0"))
	float EyeGazeProximityThreshold = 7.5f;

	/**
	 * The maximum distance between the head gaze location on the hand plane and the reference point to accept the gaze.
	 * Only used if eye gaze isn't available.
	 */
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Uxt Palm Up Constraint", meta = (EditCondition = "bRequireGaze", ClampMin = "0.0"))
	float HeadGazeProximityThreshold = 15.0f;

private:
	bool IsPalmUp(const FTransform& HeadPose, const FVector& PalmLocation, const FVector& PalmUpVector) const;
	bool IsHandFlat(EControllerHand NewHand, const FVector& PalmLocation, const FVector& PalmUpVector) const;
	bool HasEyeGaze(EControllerHand NewHand, const FTransform& HeadPose, const FVector& PalmLocation) const;

	/** Cache the gaze trigger so it only needs to be met to activate the constraint. */
	bool bGazeTriggered = false;
};
