// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtConstraintManager.h"

#include "GameFramework/Actor.h"

UxtConstraintManager::UxtConstraintManager(AActor& OwningActor) : Actor(OwningActor)
{
}

void UxtConstraintManager::ApplyScaleConstraints(FTransform& Transform, bool IsOneHanded, bool IsNear) const
{
	ApplyConstraintsForType(Transform, IsOneHanded, IsNear, EUxtTransformMode::Scaling);
}

void UxtConstraintManager::ApplyRotationConstraints(FTransform& Transform, bool IsOneHanded, bool IsNear) const
{
	ApplyConstraintsForType(Transform, IsOneHanded, IsNear, EUxtTransformMode::Rotation);
}

void UxtConstraintManager::ApplyTranslationConstraints(FTransform& Transform, bool IsOneHanded, bool IsNear) const
{
	ApplyConstraintsForType(Transform, IsOneHanded, IsNear, EUxtTransformMode::Translation);
}

void UxtConstraintManager::Initialize(FTransform& WorldPose)
{
	Actor.GetComponents<UUxtTransformConstraint>(Constraints);

	for (UUxtTransformConstraint* TransformConstraint : Constraints)
	{
		TransformConstraint->Initialize(WorldPose);
	}
}

void UxtConstraintManager::ApplyConstraintsForType(
	FTransform& Transform, bool IsOneHanded, bool IsNear, EUxtTransformMode TransformType) const
{
	int32 HandMode = static_cast<int32>(IsOneHanded ? EUxtGrabMode::OneHanded : EUxtGrabMode::TwoHanded);
	int32 InteractionMode = static_cast<int32>(IsNear ? EUxtInteractionMode::Near : EUxtInteractionMode::Far);

	for (UUxtTransformConstraint* Constraint : Constraints)
	{
		if (Constraint->GetConstraintType() == TransformType && Constraint->HandType & HandMode &&
			Constraint->InteractionMode & InteractionMode)
		{
			Constraint->ApplyConstraint(Transform);
		}
	}
}

void UxtConstraintManager::Update(const FTransform& TargetTransform)
{
	// get current list of constraints
	TArray<UUxtTransformConstraint*> UpToDateConstraints;
	Actor.GetComponents<UUxtTransformConstraint>(UpToDateConstraints);

	// iterate over new constraints and make sure we have them in our constraints cache
	// initialize and add any new constraints
	int processedCachedConstraints = 0;
	int addedConstraints = 0;
	for (UUxtTransformConstraint* NewConstraint : UpToDateConstraints)
	{
		bool foundConstraint = false;
		for (int32 i = 0; i < Constraints.Num(); ++i)
		{
			UUxtTransformConstraint* CachedConstraint = Constraints[i];
			if (NewConstraint == CachedConstraint)
			{
				foundConstraint = true;
				processedCachedConstraints++;
				break;
			}
		}

		if (!foundConstraint)
		{
			NewConstraint->Initialize(TargetTransform);
			addedConstraints++;
		}
	}

	// replace existing list with UpToDate list if there were any changes
	if (addedConstraints > 0 || processedCachedConstraints < Constraints.Num())
	{
		Constraints = UpToDateConstraints;
	}
}
