// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Interactions/UxtGrabTargetComponent.h"

#include "UxtManipulatorComponentBase.generated.h"

class UxtManipulationMoveLogic;
class UxtTwoHandManipulationRotateLogic;
class UxtTwoHandManipulationScaleLogic;
class UxtConstraintManager;

/** Event triggered when the actor's transform is updated. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtUpdateTransformDelegate, USceneComponent*, TargetComponent, FTransform, Transform);

/**
 * Base class for manipulation components that react to pointer interactions.
 *
 * This class does not modify the actor as-is. Implementations should use the provided functions
 * to compute a target transform from grabbing pointers and call the ApplyTargetTransform method
 * to actually modify the actor.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtManipulatorComponentBase : public UUxtGrabTargetComponent
{
	GENERATED_BODY()

public:
	UUxtManipulatorComponentBase();
	~UUxtManipulatorComponentBase();

	/**
	 * Translate the source transform such that grab points match targets.
	 * If more than one pointer is used then the centroid of the grab points and targets is used.
	 */
	UFUNCTION(BlueprintPure, Category = "Uxt Manipulator Component Base")
	void MoveToTargets(const FTransform& SourceTransform, FTransform& TargetTransform, bool UsePointerRotation) const;

	/**
	 * Rotates the source transform around the pivot point such that the pointers line up with current targets.
	 * If more than one pointer is used then the resulting rotation will minimize the mean square of target distances.
	 */
	UFUNCTION(BlueprintPure, Category = "Uxt Manipulator Component Base")
	void RotateAroundPivot(const FTransform& SourceTransform, const FVector& Pivot, FTransform& TargetTransform) const;

	/**
	 * Rotates the source transform around the pivot point on the given axis such that the pointers line up with current targets.
	 * If more than one pointer is used then the resulting rotation will minimize the mean square of target distances.
	 */
	UFUNCTION(BlueprintPure, Category = "Uxt Manipulator Component Base")
	void RotateAboutAxis(const FTransform& SourceTransform, const FVector& Pivot, const FVector& Axis, FTransform& TargetTransform) const;

	/**
	 * Apply a low-pass filter to the source transform location and rotation to smooth out jittering.
	 * Target transform is a exponentially weighted average of the current component transform and the source transform based on the time
	 * step.
	 */
	UFUNCTION(BlueprintPure, Category = "Uxt Manipulator Component Base")
	void SmoothTransform(
		const FTransform& SourceTransform, float LocationSmoothing, float RotationSmoothing, float DeltaSeconds,
		FTransform& TargetTransform) const;

	/**
	 * Cache the initial world space and camera space transform.
	 * Manipulation should be based on these initial transform for stable results.
	 * If bAutoSetInitialTransform is true then the initial transform is updated when grabbed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator Component Base")
	void SetInitialTransform();

	/**
	 * Apply the transform to the actor root scene component.
	 * Relative transform between the manipulator component and the root scene component is preserved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator Component Base")
	void ApplyTargetTransform(const FTransform& TargetTransform);

	/** Get the component the manipulator is targeting. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator Component Base")
	USceneComponent* GetTargetComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UxtManipulationMoveLogic* MoveLogic;                   // computes move for one and two hands
	UxtTwoHandManipulationRotateLogic* TwoHandRotateLogic; // computes rotation for two hands
	UxtTwoHandManipulationScaleLogic* TwoHandScaleLogic;   // computes scale for two hands

	UxtConstraintManager* Constraints; // constraint manager - applies constraints to transform changes
private:
	UFUNCTION()
	void OnManipulationStarted(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	UFUNCTION()
	void OnManipulationEnd(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	void UpdateManipulationLogic(int NumGrabPointers);

public:
	UPROPERTY(BlueprintAssignable, Category = "Uxt Manipulator Component Base")
	FUxtUpdateTransformDelegate OnUpdateTransform;

	UPROPERTY(BlueprintReadonly, Category = "Uxt Manipulator Component Base")
	FTransform InitialTransform;

	UPROPERTY(BlueprintReadonly, Category = "Uxt Manipulator Component Base")
	FTransform InitialCameraSpaceTransform;

	/** If true the initial transform will be set automatically when the component is grabbed. */
	UPROPERTY(EditAnywhere, Category = "Uxt Manipulator Component Base", AdvancedDisplay)
	bool bAutoSetInitialTransform = true;

	/** The component to transform, will default to the root scene component if not specified */
	USceneComponent* TransformTarget = nullptr;
};
