// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtManipulatorComponentBase.h"
#include "UxtGenericManipulatorComponent.generated.h"

/** Manipulation modes supported by the generic manipulator. */
UENUM(meta = (Bitflags))
enum class EUxtGenericManipulationMode : uint8
{
	/** Move and rotate objects with one hand. */
	OneHanded,
	/** Move, rotate, scale objects with two hands. */
	TwoHanded,
};
ENUM_CLASS_FLAGS(EUxtGenericManipulationMode)

/** Specifies how the object will rotate when it is being grabbed with one hand. */
UENUM(BlueprintType)
enum class EUxtOneHandRotationMode : uint8
{
	/** Does not rotate object as it is being moved. */
	MaintainOriginalRotation,
	/** Only works for articulated hands/controllers. Rotate object using rotation of the hand/controller, but about the object center point. Useful for inspecting at a distance. */
	RotateAboutObjectCenter,
	/** Only works for articulated hands/controllers. Rotate object as if it was being held by hand/controller. Useful for inspection. */
	RotateAboutGrabPoint,
	/** Maintains the object's original rotation for Y/Z axis to the user. */
	MaintainRotationToUser,
	/** Maintains object's original rotation to user, but makes the object vertical. Useful for bounding boxes. */
	GravityAlignedMaintainRotationToUser,
	/** Ensures object always faces the user. Useful for slates/panels. */
	FaceUser,
	/** Ensures object always faces away from user. Useful for slates/panels that are configured backwards. */
	FaceAwayFromUser,
};

/** Two-handed transformations supported by the generic manipulator. */
UENUM(meta = (Bitflags))
enum class EUxtTwoHandTransformMode : uint8
{
	/** Translation by average movement of grab points. */
	Translation,
	/** Rotation by the line between grab points. */
	Rotation,
	/** Scaling by distance between grab points. */
	Scaling,
};
ENUM_CLASS_FLAGS(EUxtTwoHandTransformMode)

/**
 * Generic manipulator that supports both one- and two-handed interactions.
 * 
 * One-handed interaction supports linear movement as well as rotation based on the orientation of the hand.
 * Rotation modes can be selected with different axes and pivot points.
 * 
 * Two-handed interaction moves the object based on the center between hands.
 * The actor can be rotated based on the line between both hands and scaled based on the distance.
 */
UCLASS(ClassGroup = UXTools, HideCategories = (Grabbable, ManipulatorComponent), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtGenericManipulatorComponent : public UUxtManipulatorComponentBase
{
	GENERATED_BODY()

public:

	UUxtGenericManipulatorComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintGetter)
	float GetSmoothing() const;
	UFUNCTION(BlueprintSetter)
	void SetSmoothing(float NewSmoothing);

protected:

	void UpdateOneHandManipulation(float DeltaSeconds);
	void UpdateTwoHandManipulation(float DeltaSeconds);

	bool GetOneHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;
	bool GetTwoHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;
	bool GetTwoHandScale(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const;

	/** Compute orientation that invariant in camera space. */
	FQuat GetViewInvariantRotation() const;

public:

	/** Enabled manipulation modes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator, meta = (Bitmask, BitmaskEnum = EUxtGenericManipulationMode))
	uint8 ManipulationModes;

	/** Mode of rotation to use while using one hand only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator)
	EUxtOneHandRotationMode OneHandRotationMode;

	/** Enabled transformations in two-handed manipulation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GenericManipulator, meta = (Bitmask, BitmaskEnum = EUxtTwoHandTransformMode))
	uint8 TwoHandTransformModes;

private:

	/** Motion smoothing factor to apply while manipulating the object.
	 *
	 * A low-pass filter is applied to the source transform location and rotation to smooth out jittering.
	 * The new actor transform is a exponentially weighted average of the current transform and the raw target transform based on the time step:
	 *
	 * T_final = Lerp( T_current, T_target, Exp(-Smoothing * DeltaSeconds) )
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetSmoothing, BlueprintSetter = SetSmoothing, Category = GenericManipulator, meta = (ClampMin = "0.0"))
	float Smoothing;
};
