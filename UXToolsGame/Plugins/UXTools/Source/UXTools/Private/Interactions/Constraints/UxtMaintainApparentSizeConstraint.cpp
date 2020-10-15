// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtMaintainApparentSizeConstraint.h"

#include "Engine/World.h"
#include "Utils/UxtFunctionLibrary.h"

void UUxtMaintainApparentSizeConstraint::Initialize(const FTransform& WorldPose)
{
	Super::Initialize(WorldPose);

	InitialDistance = FVector::Dist(WorldPose.GetLocation(), UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation());
}

EUxtTransformMode UUxtMaintainApparentSizeConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Scaling;
}

void UUxtMaintainApparentSizeConstraint::ApplyConstraint(FTransform& Transform) const
{
	const float CurrentDistance = FVector::Dist(Transform.GetLocation(), UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation());
	Transform.SetScale3D((CurrentDistance / InitialDistance) * WorldPoseOnManipulationStart.GetScale3D());
}
