// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrabbableComponent.h"
#include "ManipulatorComponentBase.generated.h"

/**
 * Base class for manipulation components that react to pointer interactions.
 *
 * This class does not modify the actor as-is. Implementations should use the provided functions
 * to compute a target transform from grabbing pointers and call the ApplyTargetTransform method
 * to actually modify the actor.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXEDREALITYTOOLS_API UManipulatorComponentBase : public UGrabbableComponent
{
	GENERATED_BODY()

public:

	/**
	 * Translate the source transform such that grab points match targets.
	 * If more than one pointer is used then the centroid of the grab points and targets is used.
	 */
	UFUNCTION(BlueprintPure)
	void MoveToTargets(const FTransform &SourceTransform, FTransform &TargetTransform) const;

	/**
	 * Rotates the source transform around the pivot point such that the pointers line up with current targets.
	 * If more than one pointer is used then the resulting rotation will minimize the mean square of target distances.
	 */
	UFUNCTION(BlueprintPure)
	void RotateAroundPivot(const FTransform &SourceTransform, const FVector &Pivot, FTransform &TargetTransform) const;

	/**
	 * Rotates the source transform around the pivot point on the given axis such that the pointers line up with current targets.
	 * If more than one pointer is used then the resulting rotation will minimize the mean square of target distances.
	 */
	UFUNCTION(BlueprintPure)
	void RotateAboutAxis(const FTransform &SourceTransform, const FVector &Pivot, const FVector &Axis, FTransform &TargetTransform) const;

	/**
	 * Cache the initial world space and camera space transform.
	 * Manipulation should be based on these initial transform for stable results.
	 * If bAutoSetInitialTransform is true then the initial transform is updated when grabbed.
	 */
	UFUNCTION(BlueprintCallable)
	void SetInitialTransform();

	/**
	 * Apply the transform to the actor root scene component.
	 * Relative transform between the manipulator component and the root scene component is preserved.
	 */
	UFUNCTION(BlueprintCallable)
	void ApplyTargetTransform(const FTransform &TargetTransform);

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void InitTransformOnFirstPointer(UGrabbableComponent *Grabbable, FGrabPointerData GrabPointer);

public:

	UPROPERTY(BlueprintReadonly)
	FTransform InitialTransform;

	UPROPERTY(BlueprintReadonly)
	FTransform InitialCameraSpaceTransform;

	/** If true the initial transform will be set automatically when the component is grabbed. */
	UPROPERTY(EditAnywhere)
	bool bAutoSetInitialTransform = true;

};
