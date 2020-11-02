// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"
#include "Interactions/UxtManipulationFlags.h"

/**
 * Manages constraints for a given object and ensures that Scale/Rotation/Translation
 * constraints are executed separately.
 */
class UxtConstraintManager
{
public:
	/** Registers all transform constraints attached to the actor. */
	UxtConstraintManager(AActor& OwningActor);

	/** Applies scale constraint to transform */
	void ApplyScaleConstraints(FTransform& Transform, bool IsOneHanded, bool IsNear) const;

	/** Applies rotation constraint to transform */
	void ApplyRotationConstraints(FTransform& Transform, bool IsOneHanded, bool IsNear) const;

	/** Applies translation constraint to transform */
	void ApplyTranslationConstraints(FTransform& Transform, bool IsOneHanded, bool IsNear) const;

	/** Initialized all registered transform constraints */
	void Initialize(FTransform& WorldPose);

	/** Compares existing registered constraints with up to date constraint list attached to actor */
	void Update(const FTransform& TargetTransform);

private:
	void ApplyConstraintsForType(FTransform& Transform, bool IsOneHanded, bool IsNear, EUxtTransformMode TransformType) const;

	TArray<UUxtTransformConstraint*> Constraints;
	const AActor& Actor;
};
