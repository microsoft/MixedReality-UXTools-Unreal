// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtFixedDistanceConstraint.h"

#include "Engine/World.h"
#include "Utils/UxtFunctionLibrary.h"

void UUxtFixedDistanceConstraint::Initialize(const FTransform& WorldPose)
{
	Super::Initialize(WorldPose);

	InitialDistance = FVector::Dist(WorldPose.GetLocation(), GetConstraintLocation());
}

EUxtTransformMode UUxtFixedDistanceConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Translation;
}

void UUxtFixedDistanceConstraint::ApplyConstraint(FTransform& Transform) const
{
	const FVector ConstraintLocation = GetConstraintLocation();

	FVector ConstraintToPose = Transform.GetLocation() - ConstraintLocation;
	ConstraintToPose = ConstraintToPose.GetSafeNormal() * InitialDistance;
	Transform.SetLocation(ConstraintLocation + ConstraintToPose);
}

FVector UUxtFixedDistanceConstraint::GetConstraintLocation() const
{
	// FComponentReference doesn't override the != operator
	if (!(ConstraintComponent == FComponentReference()))
	{
		if (USceneComponent* Component = UUxtFunctionLibrary::GetSceneComponentFromReference(ConstraintComponent, GetOwner()))
		{
			return Component->GetComponentLocation();
		}
	}

	return UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation();
}
