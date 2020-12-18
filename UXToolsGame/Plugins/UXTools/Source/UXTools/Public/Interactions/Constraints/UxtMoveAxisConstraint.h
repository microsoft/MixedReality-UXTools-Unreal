// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"
#include "Interactions/UxtManipulationFlags.h"

#include "UxtMoveAxisConstraint.generated.h"

/**
 * Component for limiting the translation axes for Manipulator
 *
 * Usage:
 * Attach to actor that the constraint should be applied to.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtMoveAxisConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual EUxtTransformMode GetConstraintType() const;
	virtual void ApplyConstraint(FTransform& Transform) const;

public:
	/** Defines the axis the movement constraint should be applied to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint|Move Axis", meta = (Bitmask, BitmaskEnum = EUxtAxisFlags))
	int32 ConstraintOnMovement = 0;

	/** Use local or global space for constraint calculations*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint|Move Axis")
	bool bUseLocalSpaceForConstraint = false;
};
