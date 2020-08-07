// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "CoreMinimal.h"

#include "UxtInteractionMode.generated.h"

/*** Interaction modes supported by the generic manipulator. */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EUxtInteractionMode : uint8
{
	None = 0 UMETA(Hidden),
	/** Move, rotate and scale objects with near interaction. */
	Near = 1 << 0,
	/** Move, rotate and scale objects with far interaction. */
	Far = 1 << 1,
};
ENUM_CLASS_FLAGS(EUxtInteractionMode)
