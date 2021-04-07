// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtManipulationFlags.h"
#include "UxtManipulatorComponentBase.h"

#include "UxtGenericManipulatorComponent.generated.h"
/**
 * Generic manipulator that supports both one- and two-handed interactions.
 *
 * One-handed interaction supports linear movement as well as rotation based on the orientation of the hand.
 * Rotation modes can be selected with different axes and pivot points.
 *
 * Two-handed interaction moves the object based on the center between hands.
 * The actor can be rotated based on the line between both hands and scaled based on the distance.
 */
UCLASS(ClassGroup = "UXTools", HideCategories = (Grabbable, ManipulatorComponent), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtGenericManipulatorComponent : public UUxtManipulatorComponentBase
{
	GENERATED_BODY()

public:
	UUxtGenericManipulatorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void UpdateOneHandManipulation(float DeltaSeconds);
	void UpdateTwoHandManipulation(float DeltaSeconds);

	bool GetOneHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;
	bool GetTwoHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;
	bool GetTwoHandScale(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;

	/** Compute orientation that invariant in camera space. */
	FQuat GetViewInvariantRotation() const;

	virtual void BeginPlay() override;

public:
	/** Mode of rotation to use while using one hand only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Generic Manipulator")
	EUxtOneHandRotationMode OneHandRotationMode;

	/** Enabled transformations in two-handed manipulation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Generic Manipulator", meta = (Bitmask, BitmaskEnum = EUxtTransformMode))
	int32 TwoHandTransformModes;

	/** Controls the object's behavior when physics its being simulated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Generic Manipulator", meta = (Bitmask, BitmaskEnum = EUxtReleaseBehavior))
	int32 ReleaseBehavior;

	/** The component to transform, will default to the root scene component if not specified */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Generic Manipulator", AdvancedDisplay, meta = (UseComponentPicker, AllowedClasses = "SceneComponent"))
	FComponentReference TargetComponent;

	/**
	 * Interpolation time for smoothed movement while manipulating.
	 * Set to zero to disable smoothing.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Generic Manipulator", meta = (ClampMin = "0.0"))
	float LerpTime = 0.08f;

private:
	bool IsNearManipulation() const;

	UFUNCTION(Category = "Uxt Generic Manipulator")
	void OnGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	UFUNCTION(Category = "Uxt Generic Manipulator")
	void OnRelease(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	/** Was the target simulating physics */
	bool bWasSimulatingPhysics = false;
};
