// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UxtInteractionMode.generated.h"

/** Interaction modes supported */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EUxtInteractionMode : uint8
{
	None = 0 UMETA(Hidden),
	/** Interact with poke and grab targets (see IUxtPokeTarget and IUxtGrabTarget) */
	Near = 1 << 0,
	/** Interact with far targets (see IUxtFarTarget) */
	Far = 1 << 1,
};
ENUM_CLASS_FLAGS(EUxtInteractionMode)
