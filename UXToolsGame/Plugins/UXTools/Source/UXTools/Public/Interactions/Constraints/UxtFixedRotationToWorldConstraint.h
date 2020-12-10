// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"
#include "Interactions/UxtManipulationFlags.h"

#include "UxtFixedRotationToWorldConstraint.generated.h"

/**
 * Component for fixing the rotation of a manipulated object relative to the world
 *
 * Usage:
 * Attach to actor that the constraint should be applied to.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFixedRotationToWorldConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual EUxtTransformMode GetConstraintType() const;
	virtual void ApplyConstraint(FTransform& Transform) const;
};
