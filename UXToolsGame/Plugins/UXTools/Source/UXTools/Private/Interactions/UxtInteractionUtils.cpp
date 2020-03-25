// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtInteractionUtils.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

bool FUxtInteractionUtils::GetDefaultClosestPoint(const AActor* Actor, const FVector& Point, FVector& OutPointOnSurface)
{
	OutPointOnSurface = Point;
	float ClosestPointDistanceSqr = -1.f;

	TInlineComponentArray<UPrimitiveComponent*> Components;
	Actor->GetComponents(Components);

	// This is exactly what happens inside AActor::ActorGetDistanceToCollision except we're skipping the collision response check
	// because the pointer will usually be configured to overlap with this actor's primitives, not block.
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UPrimitiveComponent* Primitive = Components[ComponentIndex];
		if (Primitive->IsRegistered() && Primitive->IsCollisionEnabled())
		{
			FVector ClosestPoint;
			float DistanceSqr = -1.f;

			if (!Primitive->GetSquaredDistanceToCollision(Point, DistanceSqr, ClosestPoint))
			{
				// Invalid result, impossible to be better than ClosestPointDistance
				continue;
			}

			if ((ClosestPointDistanceSqr < 0.f) || (DistanceSqr < ClosestPointDistanceSqr))
			{
				ClosestPointDistanceSqr = DistanceSqr;
				OutPointOnSurface = ClosestPoint;

				// If we're inside collision, we're not going to find anything better, so abort search we've got our best find.
				if (DistanceSqr <= KINDA_SMALL_NUMBER)
				{
					break;
				}
			}
		}
	}

	return ClosestPointDistanceSqr >= 0.f;
}
