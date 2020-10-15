// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtRotationAxisConstraint.h"

EUxtTransformMode UUxtRotationAxisConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Rotation;
}

void UUxtRotationAxisConstraint::ApplyConstraint(FTransform& Transform) const
{
	FQuat InverseRotation = WorldPoseOnManipulationStart.GetRotation().Inverse();
	FQuat RotationFromManipulationStart = Transform.GetRotation() * InverseRotation;

	FVector Eulers = RotationFromManipulationStart.Euler();
	if (ConstraintOnRotation & static_cast<int32>(EUxtAxisFlags::X))
	{
		Eulers.X = 0;
	}
	if (ConstraintOnRotation & static_cast<int32>(EUxtAxisFlags::Y))
	{
		Eulers.Y = 0;
	}
	if (ConstraintOnRotation & static_cast<int32>(EUxtAxisFlags::Z))
	{
		Eulers.Z = 0;
	}

	Transform.SetRotation(
		bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetRotation() * FQuat::MakeFromEuler(Eulers)
									: FQuat::MakeFromEuler(Eulers) * WorldPoseOnManipulationStart.GetRotation());
}
