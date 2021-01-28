// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtConstrainableComponent.h"

#include "UXTools.h"

bool UUxtConstrainableComponent::GetAutoDetectConstraints() const
{
	return bAutoDetectConstraints;
}

void UUxtConstrainableComponent::SetAutoDetectConstraints(bool bNewAutoDetectConstraints)
{
	bAutoDetectConstraints = bNewAutoDetectConstraints;
	UpdateActiveConstraints();
}

const TArray<FComponentReference>& UUxtConstrainableComponent::GetSelectedConstraints() const
{
	return SelectedConstraints;
}

void UUxtConstrainableComponent::AddConstraint(const FComponentReference& NewConstraint)
{
	if (bAutoDetectConstraints)
	{
		UE_LOG(UXTools, Warning, TEXT("Manually adding a constraint to a UxtConstrainableComponent using automatic constraint detection."));
	}

	SelectedConstraints.Add(NewConstraint);
	UpdateActiveConstraints();
}

void UUxtConstrainableComponent::RemoveConstraint(const FComponentReference& NewConstraint)
{
	if (bAutoDetectConstraints)
	{
		UE_LOG(
			UXTools, Warning,
			TEXT("Manually removing a constraint from a UxtConstrainableComponent using automatic constraint detection."));
	}

	SelectedConstraints.Remove(NewConstraint);
	UpdateActiveConstraints();
}

void UUxtConstrainableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAutoDetectConstraints)
	{
		UpdateActiveConstraints();
	}
}

void UUxtConstrainableComponent::InitializeConstraints(USceneComponent* NewTargetComponent)
{
	check(NewTargetComponent);
	TargetComponent = NewTargetComponent;
	ActiveConstraints = GetConstraints();

	for (UUxtTransformConstraint* Constraint : ActiveConstraints)
	{
		Constraint->Initialize(TargetComponent->GetComponentTransform());
	}
}

void UUxtConstrainableComponent::ApplyConstraints(
	FTransform& Transform, EUxtTransformMode TransformMode, bool bIsOneHanded, bool bIsNear) const
{
	const int32 GrabMode = static_cast<int32>(bIsOneHanded ? EUxtGrabMode::OneHanded : EUxtGrabMode::TwoHanded);
	const int32 InteractionMode = static_cast<int32>(bIsNear ? EUxtInteractionMode::Near : EUxtInteractionMode::Far);

	for (const UUxtTransformConstraint* Constraint : ActiveConstraints)
	{
		if (Constraint->GetConstraintType() == TransformMode && Constraint->HandType & GrabMode &&
			Constraint->InteractionMode & InteractionMode)
		{
			Constraint->ApplyConstraint(Transform);
		}
	}
}

TArray<UUxtTransformConstraint*> UUxtConstrainableComponent::GetConstraints() const
{
	TArray<UUxtTransformConstraint*> Constraints;

	if (bAutoDetectConstraints)
	{
		GetOwner()->GetComponents<UUxtTransformConstraint>(Constraints);
	}
	else
	{
		Constraints.Reserve(SelectedConstraints.Num());

		for (const FComponentReference& ConstraintReference : SelectedConstraints)
		{
			if (UUxtTransformConstraint* Constraint = Cast<UUxtTransformConstraint>(ConstraintReference.GetComponent(GetOwner())))
			{
				Constraints.Add(Constraint);
			}
		}
	}

	return Constraints;
}

void UUxtConstrainableComponent::UpdateActiveConstraints()
{
	if (!TargetComponent)
	{
		// Don't update constraints if we don't have a target component.
		return;
	}

	int NewConstraints = 0;
	int ExistingConstraints = 0;
	TArray<UUxtTransformConstraint*> Constraints = GetConstraints();

	for (UUxtTransformConstraint* Constraint : Constraints)
	{
		if (ActiveConstraints.Contains(Constraint))
		{
			ExistingConstraints++;
		}
		else
		{
			Constraint->Initialize(TargetComponent->GetComponentTransform());
			NewConstraints++;
		}
	}

	if (NewConstraints > 0 || ExistingConstraints < ActiveConstraints.Num())
	{
		ActiveConstraints = Constraints;
	}
}
