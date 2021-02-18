// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtRotationAxisConstraint.h"

EUxtTransformMode UUxtRotationAxisConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Rotation;
}

void UUxtRotationAxisConstraint::ApplyConstraint(FTransform& Transform) const
{
	FQuat RotationSpace = WorldPoseOnManipulationStart.GetRotation();
	FQuat DeltaRotation = RotationSpace.Inverse() * Transform.GetRotation();

	// As DeltaRotation is in World space, unrotating the reference vectors aligns them to the actual World axes
	const FVector RefX = bUseLocalSpaceForConstraint ? FVector::ForwardVector : RotationSpace.UnrotateVector(FVector::ForwardVector);
	const FVector RefY = bUseLocalSpaceForConstraint ? FVector::RightVector : RotationSpace.UnrotateVector(FVector::RightVector);
	const FVector RefZ = bUseLocalSpaceForConstraint ? FVector::UpVector : RotationSpace.UnrotateVector(FVector::UpVector);

	FVector TwistAxis = FVector::ZeroVector;
	switch (AllowedAxis)
	{
	case EUxtAxis::None:
		TwistAxis = FVector::ZeroVector;
		break;
	case EUxtAxis::X:
		TwistAxis = FVector::CrossProduct(RefY, RefZ);
		break;
	case EUxtAxis::Y:
		TwistAxis = FVector::CrossProduct(RefX, RefZ);
		break;
	case EUxtAxis::Z:
		TwistAxis = FVector::CrossProduct(RefX, RefY);
		break;
	default:
		break;
	}

	FQuat Swing, Twist;
	DeltaRotation.ToSwingTwist(TwistAxis.GetSafeNormal(), Swing, Twist);

	Transform.SetRotation(RotationSpace * Twist);
}
