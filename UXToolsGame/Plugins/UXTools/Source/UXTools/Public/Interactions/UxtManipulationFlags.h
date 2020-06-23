// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CoreMinimal.h"
#include "UxtManipulationFlags.generated.h"

/** Manipulation modes supported by the generic manipulator. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EUxtGenericManipulationMode : uint8
{
	None = 0 UMETA(Hidden),
	/** Move and rotate objects with one hand. */
	OneHanded = 1 << 0,
	/** Move, rotate, scale objects with two hands. */
	TwoHanded = 1 << 1,
};
ENUM_CLASS_FLAGS(EUxtGenericManipulationMode)

/*** Interaction modes supported by the generic manipulator. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EUxtInteractionMode : uint8
{
	None = 0 UMETA(Hidden),
	/** Move, rotate and scale objects with near interaction. */
	Near = 1 << 0,
	/** Move, rotate and scale objects with far interaction. */
	Far = 1 << 1,
};
ENUM_CLASS_FLAGS(EUxtInteractionMode)


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
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EUxtTransformMode : uint8
{
	None = 0 UMETA(Hidden),
	/** Translation by average movement of grab points. */
	Translation = 1 << 0,
	/** Rotation by the line between grab points. */
	Rotation = 1 << 1,
	/** Scaling by distance between grab points. */
	Scaling = 1 << 2,
};

ENUM_CLASS_FLAGS(EUxtTransformMode)


///** Flags used to represent a set of 3D axes. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EUxtAxisFlags : uint8
{
	None = 0 UMETA(Hidden),
	X = 1 << 0,
	Y = 1 << 1,
	Z = 1 << 2,
};
ENUM_CLASS_FLAGS(EUxtAxisFlags)
