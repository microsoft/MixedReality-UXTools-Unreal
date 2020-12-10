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

	/** Action that this affordance will perform when grabbed */
	EUxtAffordanceAction GetAction() const;

	/**
	 * Location and rotation of the affordance in world space, based on the root transform.
	 * Root transform scale is not included in the result.
	 */
	void GetWorldLocationAndRotation(const FBox& Bounds, const FTransform& RootTransform, FVector& OutLocation, FQuat& OutRotation) const;

	/** Preset type of the affordance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Affordance Config")
	EUxtAffordancePlacement Placement = EUxtAffordancePlacement::Center;

	/** The Euler orientation of the affordance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Affordance Config")
	FVector Rotation = FVector::ZeroVector;
};

/** Data asset that stores the configuration of a bounds control. */
UCLASS(BlueprintType, Category = UXTools)
class UXTOOLS_API UUxtBoundsControlConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Uxt Bounds Control Config")
	TArray<FUxtAffordanceConfig> Affordances;

	/** Whether this configuration is intended to be used for slate elements */
	UPROPERTY(EditAnywhere, Category = "Uxt Bounds Control Config")
	bool bIsSlate = false;

	/** Whether this configuration transforms the target uniformly or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control Config")
	bool bUniformScaling = true;
};
