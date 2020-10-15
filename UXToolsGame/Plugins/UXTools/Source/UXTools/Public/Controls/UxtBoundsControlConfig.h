// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"
#include "Interactions/UxtManipulationFlags.h"

#include "UxtBoundsControlConfig.generated.h"

/** Supported placements for affordances. */
UENUM()
enum class EUxtAffordancePlacement : uint8
{
	Center,
	FaceFront,
	FaceBack,
	FaceRight,
	FaceLeft,
	FaceTop,
	FaceBottom,
	EdgeFrontRight,
	EdgeFrontLeft,
	EdgeFrontTop,
	EdgeFrontBottom,
	EdgeBackRight,
	EdgeBackLeft,
	EdgeBackTop,
	EdgeBackBottom,
	EdgeRightTop,
	EdgeRightBottom,
	EdgeLeftTop,
	EdgeLeftBottom,
	CornerFrontRightTop,
	CornerFrontRightBottom,
	CornerFrontLeftTop,
	CornerFrontLeftBottom,
	CornerBackRightTop,
	CornerBackRightBottom,
	CornerBackLeftTop,
	CornerBackLeftBottom,
};

/** Defines the kind of actor that should be spawned for an affordance. */
UENUM()
enum class EUxtAffordanceKind : uint8
{
	Center,
	Face,
	Edge,
	Corner,
};

/** Defines which effect moving an affordance has on the bounding box. */
UENUM()
enum class EUxtAffordanceAction : uint8
{
	/** Move only one side of the bounding box. */
	Resize,
	/** Move both sides of the bounding box. */
	Translate,
	/** Scale the bounding box, moving both sides in opposite directions. */
	Scale,
	/** Rotate the bounding box about its center point. */
	Rotate,
};

/** Affordances are grabbable actors placed on the bounding box which enable interaction. */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtAffordanceConfig
{
	GENERATED_BODY()

	/** Location of the affordance in normalized bounding box space (-1..1). */
	FVector GetBoundsLocation() const;

	/** Rotation of the affordance in bounding box space. */
	FRotator GetBoundsRotation() const;

	/** Kind of actor class to use.
	 * The matching actor class from the bounding box component will be used.
	 */
	EUxtAffordanceKind GetAffordanceKind() const;

	/** Constraint matrix defining possible movement directions or rotation axes.
	 * Drag vectors during interaction are multiplied with this matrix.
	 * If UniformAction is true the action will apply on all axes equally.
	 * LockedAxes flags place additional constraints on local axes.
	 */
	FMatrix GetConstraintMatrix(int32 LockedAxes) const;

	/**
	 * Location and rotation of the affordance in world space, based on the root transform.
	 * Root transform scale is not included in the result.
	 */
	void GetWorldLocationAndRotation(const FBox& Bounds, const FTransform& RootTransform, FVector& OutLocation, FQuat& OutRotation) const;

	/** Preset type of the affordance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	EUxtAffordancePlacement Placement = EUxtAffordancePlacement::Center;

	/** Action to perform when the affordance is grabbed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	EUxtAffordanceAction Action = EUxtAffordanceAction::Resize;

	/** Apply action in all directions uniformly.
	 * If true transform is changed equally along every unconstrained axis.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	bool bUniformAction = true;
};

/** Data asset that stores the configuration of a bounds control. */
UCLASS(BlueprintType, Category = UXTools)
class UXTOOLS_API UUxtBoundsControlConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = BoundsControl)
	TArray<FUxtAffordanceConfig> Affordances;

	/** Locked axes when changing bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl, meta = (Bitmask, BitmaskEnum = EUxtAxisFlags))
	int32 LockedAxes = 0;
};
