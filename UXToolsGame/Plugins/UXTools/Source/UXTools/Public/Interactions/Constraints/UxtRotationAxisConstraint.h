// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"
#include "Interactions/UxtManipulationFlags.h"

#include "UxtRotationAxisConstraint.generated.h"

/**
 * Component for limiting the rotation axes for Manipulator
 *
 * Usage:
 * Attach to actor that the constraint should be applied to.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtRotationAxisConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual EUxtTransformMode GetConstraintType() const;
	virtual void ApplyConstraint(FTransform& Transform) const;

public:
	/** Defines the axis the rotation constraint should be applied to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint|Rotation Axis", meta = (Bitmask, BitmaskEnum = EUxtAxisFlags))
	int32 ConstraintOnRotation = 0;

	/** Use local or global space for constraint calculations*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint|Rotation Axis")
	bool bUseLocalSpaceForConstraint = false;
};
