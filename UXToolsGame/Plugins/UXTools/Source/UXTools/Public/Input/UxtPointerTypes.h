// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <stdint.h>

#include "UxtPointerTypes.generated.h"

/** Data for a pointer that is interacting with a target. */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtPointerInteractionData
{
	GENERATED_BODY()

	/** Location of the pointer in world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pointer Interaction")
	FVector Location;

	/** Rotation of the pointer in world space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pointer Interaction")
	FQuat Rotation;
};