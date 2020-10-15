// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtFaceUserConstraint.h"

#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "Utils/UxtFunctionLibrary.h"

EUxtTransformMode UUxtFaceUserConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Translation;
}

void UUxtFaceUserConstraint::ApplyConstraint(FTransform& Transform) const
{
	FVector DirectionToTarget = Transform.GetLocation() - UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation();
	FQuat OrientationToUser = FRotationMatrix::MakeFromXZ(bFaceAway ? DirectionToTarget : -DirectionToTarget, FVector::UpVector).ToQuat();
	Transform.SetRotation(OrientationToUser);
}
