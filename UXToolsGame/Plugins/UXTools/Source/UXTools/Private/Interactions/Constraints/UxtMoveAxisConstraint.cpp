// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtMoveAxisConstraint.h"

EUxtTransformMode UUxtMoveAxisConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Translation;
}

void UUxtMoveAxisConstraint::ApplyConstraint(FTransform& Transform) const
{
	FQuat InverseRotation = WorldPoseOnManipulationStart.GetRotation().Inverse();
	FVector Position = Transform.GetLocation();

	if (ConstraintOnMovement & static_cast<int32>(EUxtAxisFlags::X))
	{
		if (bUseLocalSpaceForConstraint)
		{
			Position = InverseRotation * Position;
			Position.X = (InverseRotation * WorldPoseOnManipulationStart.GetLocation()).X;
			Position = WorldPoseOnManipulationStart.GetRotation() * Position;
		}
		else
		{
			Position.X = WorldPoseOnManipulationStart.GetLocation().X;
		}
	}

	if (ConstraintOnMovement & static_cast<int32>(EUxtAxisFlags::Y))
	{
		if (bUseLocalSpaceForConstraint)
		{
			Position = InverseRotation * Position;
			Position.Y = (InverseRotation * WorldPoseOnManipulationStart.GetLocation()).Y;
			Position = WorldPoseOnManipulationStart.GetRotation() * Position;
		}
		else
		{
			Position.Y = WorldPoseOnManipulationStart.GetLocation().Y;
		}
	}

	if (ConstraintOnMovement & static_cast<int32>(EUxtAxisFlags::Z))
	{
		if (bUseLocalSpaceForConstraint)
		{
			Position = InverseRotation * Position;
			Position.Z = (InverseRotation * WorldPoseOnManipulationStart.GetLocation()).Z;
			Position = WorldPoseOnManipulationStart.GetRotation() * Position;
		}
		else
		{
			Position.Z = WorldPoseOnManipulationStart.GetLocation().Z;
		}
	}

	Transform.SetLocation(Position);
}
