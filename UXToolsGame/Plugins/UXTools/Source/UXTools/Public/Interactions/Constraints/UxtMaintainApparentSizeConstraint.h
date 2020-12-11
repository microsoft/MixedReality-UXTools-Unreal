// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"

#include "UxtMaintainApparentSizeConstraint.generated.h"

/**
 * Constraint to maintain the apparent size of the object to the user.

 * Usage:
 * Attach to actor that the constraint should be applied to.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtMaintainApparentSizeConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual void Initialize(const FTransform& WorldPose) override;
	virtual EUxtTransformMode GetConstraintType() const override;
	virtual void ApplyConstraint(FTransform& Transform) const override;

private:
	/** The initial distance from the object to the head. */
	float InitialDistance;
};
