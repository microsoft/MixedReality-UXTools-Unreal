// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Interactions/Constraints/UxtTransformConstraint.h"
#include "Utils/UxtFunctionLibrary.h"
#include "GameFramework/Actor.h"

void UUxtTransformConstraint::Initialize(const FTransform& WorldPose)
{
	WorldPoseOnManipulationStart = WorldPose;
}

