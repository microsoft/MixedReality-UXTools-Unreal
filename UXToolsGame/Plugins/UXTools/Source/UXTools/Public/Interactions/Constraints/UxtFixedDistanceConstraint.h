// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"

#include "UxtFixedDistanceConstraint.generated.h"

/**
 * Constraint to fix the object distance from another object.
 *
 * Usage:
 * Attach to actor that the constraint should be applied to.
 * If necessary, configure the object to constrain to. This will default to the head if not set.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFixedDistanceConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual void Initialize(const FTransform& WorldPose) override;
	virtual EUxtTransformMode GetConstraintType() const override;
	virtual void ApplyConstraint(FTransform& Transform) const override;

public:
	/** Component to fix distance to. Defaults to the head. */
	UPROPERTY(EditAnywhere, Category = "Uxt Constraint|Fixed Distance")
	FComponentReference ConstraintComponent;

private:
	/** Get the location of the constraint component, or the head if not set. */
	FVector GetConstraintLocation() const;

	/** The distance to the object when it was grabbed. */
	float InitialDistance;
};
