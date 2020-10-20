// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/Constraints/UxtFixedRotationToUserConstraint.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UUxtFixedRotationToUserConstraint::Initialize(const FTransform& WorldPose)
{
	Super::Initialize(WorldPose);

	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	const FRotator CameraRotation = CameraManager->GetCameraRotation();

	StartObjectRotationCameraSpace = CameraRotation.Quaternion().Inverse() * WorldPose.GetRotation();
}

EUxtTransformMode UUxtFixedRotationToUserConstraint::GetConstraintType() const
{
	return EUxtTransformMode::Rotation;
}

void UUxtFixedRotationToUserConstraint::ApplyConstraint(FTransform& Transform) const
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	FRotator CameraRotation = CameraManager->GetCameraRotation();
	if (bExcludeRoll)
	{
		CameraRotation.Roll = 0.0f;
	}

	Transform.SetRotation(CameraRotation.Quaternion() * StartObjectRotationCameraSpace);
}
