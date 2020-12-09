// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtRotationAxisConstraint.h"

namespace
{
	/**
	 * Constrains the rotation around a specific axis.
	 *
	 * It rotates the @ref ReferenceVec by @ref OriginalRotation and then projects the result onto the plane defined by @ref PlaneNormal.
	 * The rotation between the reference and projected vectors is "substracted" from the original rotation, effectively constraining the
	 * rotation only around the desired axis.
	 */
	FQuat ConstrainRotationOnPlane(const FQuat& OriginalRotation, const FVector& ReferenceVec, const FVector& PlaneNormal)
	{
		const FVector RotatedReferenceVec = OriginalRotation.RotateVector(ReferenceVec);
		const FVector ProjectedVec = FVector::VectorPlaneProject(RotatedReferenceVec, PlaneNormal);
		return OriginalRotation * FQuat::FindBetweenVectors(ReferenceVec, ProjectedVec).GetNormalized().Inverse();
	}
} // namespace

EUxtTransformMode UUxtRotationAxisConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Rotation;
}

void UUxtRotationAxisConstraint::ApplyConstraint(FTransform& Transform) const
{
	FQuat InverseRotation = WorldPoseOnManipulationStart.GetRotation().Inverse();
	FQuat RotationFromManipulationStart = InverseRotation * Transform.GetRotation();

	if (ConstraintOnRotation & static_cast<int32>(EUxtAxisFlags::X))
	{
		const FVector ReferenceVec = bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetUnitAxis(EAxis::Z) : FVector::UpVector;
		const FVector PlaneNormal =
			bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetUnitAxis(EAxis::X) : FVector::ForwardVector;
		RotationFromManipulationStart = ConstrainRotationOnPlane(RotationFromManipulationStart, ReferenceVec, PlaneNormal);
	}
	if (ConstraintOnRotation & static_cast<int32>(EUxtAxisFlags::Y))
	{
		const FVector ReferenceVec =
			bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetUnitAxis(EAxis::X) : FVector::ForwardVector;
		const FVector PlaneNormal = bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetUnitAxis(EAxis::Y) : FVector::RightVector;
		RotationFromManipulationStart = ConstrainRotationOnPlane(RotationFromManipulationStart, ReferenceVec, PlaneNormal);
	}
	if (ConstraintOnRotation & static_cast<int32>(EUxtAxisFlags::Z))
	{
		const FVector ReferenceVec =
			bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetUnitAxis(EAxis::X) : FVector::ForwardVector;
		const FVector PlaneNormal = bUseLocalSpaceForConstraint ? WorldPoseOnManipulationStart.GetUnitAxis(EAxis::Z) : FVector::UpVector;
		RotationFromManipulationStart = ConstrainRotationOnPlane(RotationFromManipulationStart, ReferenceVec, PlaneNormal);
	}

	Transform.SetRotation(WorldPoseOnManipulationStart.GetRotation() * RotationFromManipulationStart);
}
