// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

struct FUxtBoundsControlAffordanceInfo;


/** Possible presets for common bounding box configurations. */
UENUM()
enum class EUxtBoundsControlPreset : uint8
{
	/** Uniform resizing with corners and rotation with edges. */
	Default,
	/** Only front corners and edges are shown, all resize */
	Slate2D,
	/** Full set of affordances, all resizing. */
	AllResize,
	/** Full set of affordances, all translating. */
	AllTranslate,
	/** Full set of affordances, all scaling. */
	AllScale,
	/** Full set of affordances, all rotating. */
	AllRotate,
};


struct FUxtBoundsControlPresetUtils
{
	/** Get the list of affordances contained in a preset. */
	static const TArray<FUxtBoundsControlAffordanceInfo> &GetPresetAffordances(EUxtBoundsControlPreset Preset);
};
