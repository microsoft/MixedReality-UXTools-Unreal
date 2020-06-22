// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UxtFollowComponent.generated.h"

UENUM(BlueprintType)
enum EUxtFollowOrientBehavior
{
	/** Billboard toward the camera */
	FaceCamera	UMETA(DisplayName = "FaceCamera"),
	/** Do not billboard unless one of three conditions are met: Angular Clamp, Distance Clamp, or camera leaves OrientToCameraDeadzoneDegrees */
	WorldLock	UMETA(DisplayName = "WorldLock"),
};

/**
 * The follow component has three different constraints that keeps its owner in front of the camera: Angular
 * Clamp, Distance Clamp, and Orientation. The combination of Angular and Distance Clamp creates a
 * frustum in front of the camera where its owner can be. If its owner is outside that frustum
 * it is adjusted.
 *
 * Angular Clamp: The objective of this constraint is to ensure that the reference forward vector remains
 * within the bounds set by the leashing parameters. To do this, determine the angles between toTarget
 * and the leashing bounds about the global Z-axis and the reference's Y-axis. If the toTarget falls
 * within the leashing bounds, then we don't have to modify it. Otherwise, we apply a correction
 * rotation to bring it within bounds. This will ensure that the its owner stays within the
 * top, bottom, right and left planes of the frustum.
 *
 * Distance Clamp: The objective of this constraint is to ensure that the following actor stays within bounds
 * set by the distance parameters. To do this, we measure the current distance from the camera to the
 * its owner. If the distance is within the MinimumDistance and MaximumDistance then we don't have to
 * modify it. Otherwise, we push away or pull in the its owner along the reference forward vector.
 * This will ensure that the its owner stays within the near and far planes of the frustum.
 *
 * Orientation: The two options provided are constant FaceCamera or WorldLock. While world locked there are
 * three conditions that will cause the its owner to face the camera:
 * 	Angular Clamps
 * 	Distance Clamps
 * 	The angle between the forward vector of the its owner and toTarget vector (vector between
 * 		the camera and the its owner) is larger than dead zone angle parameter
 */
UCLASS(ClassGroup = UXTools, meta=(BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFollowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUxtFollowComponent();

	/** Force the owner to recenter in the camera's field of view. */
	UFUNCTION(BlueprintCallable, Category = FollowMethods)
	void Recenter();

public:

	/** Actor that this component will follow. If null, this component will follow the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	AActor* ActorToFollow;

	/** Orientation Type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	TEnumAsByte<EUxtFollowOrientBehavior> OrientationType = EUxtFollowOrientBehavior::WorldLock;

	/** Rate at which its owner will move toward default distance when angular leashing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float MoveToDefaultDistanceLerpTime = 10.0f;

	/** Min distance from eye to position its owner around, i.e. the sphere radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float MinimumDistance = 50.0f;

	/** Max distance from eye to its owner */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float MaximumDistance = 100.0f;

	/** Default distance from eye to position its owner around, i.e. the sphere radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float DefaultDistance = 75.0f;

	/** The horizontal angle from the camera forward axis to the owner will not exceed this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float MaxViewHorizontalDegrees = 30.0f;

	/** The vertical angle from the camera forward axis to the owner will not exceed this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float MaxViewVerticalDegrees = 30.0f;

	/** The owner will not reorient until the angle between its forward vector and the vector to the camera is greater than this value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float OrientToCameraDeadzoneDegrees = 60.0f;

	/** Option to ignore angle clamping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	bool bIgnoreAngleClamp = false;

	/** Option to ignore distance clamping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	bool bIgnoreDistanceClamp = false;

	/** Option to ignore the pitch and roll of the camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	bool bIgnoreCameraPitchAndRoll = false;

	/** Option to ignore interpolation between follow poses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	bool bInterpolatePose = true;

	/** Pitch offset from camera (relative to Max Distance) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float PitchOffset = 0.0f;

	/** Max vertical distance between the owner and camera */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FollowParameters)
	float VerticalMaxDistance = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

private:
	void UpdateLeashing();
	void UpdateTransformToGoal(bool bSkipInterpolation, float DeltaTime = 0);

private:

	FTransform GoalTransform;
	FTransform WorkingTransform;

	bool bRecenterNextUpdate = true;
};
