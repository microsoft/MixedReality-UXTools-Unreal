// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtInteractionUtils.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

bool FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface, float& OutDistanceSqr)
{
	OutPointOnSurface = Point;
	OutDistanceSqr = -1.f;

	if (Primitive->IsRegistered() && Primitive->IsCollisionEnabled())
	{
		FVector ClosestPoint;
		float DistanceSqr = -1.f;

		if (Primitive->GetSquaredDistanceToCollision(Point, DistanceSqr, ClosestPoint))
		{
			OutPointOnSurface = ClosestPoint;
			OutDistanceSqr = DistanceSqr;
			return true;
		}
	}

	return false;
}
