// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class AActor;

class FUxtInteractionUtils
{
public:

	/** Calculates the point on the target surface that is closest to the point passed in.
	 *  Return value indicates whether a point was found.
	 */
	static bool GetDefaultClosestPointOnPrimitive(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface, float& OutDistanceSqr);

};
