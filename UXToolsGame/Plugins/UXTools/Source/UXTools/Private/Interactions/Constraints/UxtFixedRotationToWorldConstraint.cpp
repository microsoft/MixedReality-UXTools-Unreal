// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtFixedRotationToWorldConstraint.h"

EUxtTransformMode UUxtFixedRotationToWorldConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Rotation;
}

void UUxtFixedRotationToWorldConstraint::ApplyConstraint(FTransform& Transform) const
{
	Transform.SetRotation(WorldPoseOnManipulationStart.GetRotation());
}
