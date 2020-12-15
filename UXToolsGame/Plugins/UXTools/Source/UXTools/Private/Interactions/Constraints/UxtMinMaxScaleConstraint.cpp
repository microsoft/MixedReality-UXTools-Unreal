// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtMinMaxScaleConstraint.h"

EUxtTransformMode UUxtMinMaxScaleConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Scaling;
}

void UUxtMinMaxScaleConstraint::ApplyConstraint(FTransform& Transform) const
{
	FVector ConstrainedScale = Transform.GetScale3D();
	ConstrainedScale.X = FMath::Clamp(ConstrainedScale.X, MinScaleVec.X, MaxScaleVec.X);
	ConstrainedScale.Y = FMath::Clamp(ConstrainedScale.Y, MinScaleVec.Y, MaxScaleVec.Y);
	ConstrainedScale.Z = FMath::Clamp(ConstrainedScale.Z, MinScaleVec.Z, MaxScaleVec.Z);
	Transform.SetScale3D(ConstrainedScale);
}

void UUxtMinMaxScaleConstraint::Initialize(const FTransform& WorldPose)
{
	FVector LocalMinScaleVec(MinScale);
	FVector LocalMaxScaleVec(MaxScale);
	if (bRelativeToInitialScale)
	{
		LocalMinScaleVec *= WorldPose.GetScale3D();
		LocalMaxScaleVec *= WorldPose.GetScale3D();
	}
	MinScaleVec = LocalMinScaleVec;
	MaxScaleVec = LocalMaxScaleVec;
}
