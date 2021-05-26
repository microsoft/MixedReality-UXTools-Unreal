// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtScrollingObjectCollectionComponentVisualizer.h"

#include "UXToolsEditor.h"

#include "Controls/UxtScrollingObjectCollectionComponent.h"

#include <SceneManagement.h>

#include <Components/BoxComponent.h>

void FUxtScrollingObjectCollectionComponentVisualizer::DrawVisualization(
	const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UUxtScrollingObjectCollectionComponent* Collection = Cast<const UUxtScrollingObjectCollectionComponent>(Component))
	{
		if (Collection->IsVisible())
		{
			// Draw the cell boundaries.
			const FTransform& Transform = Collection->GetComponentTransform();
			const FVector Center = Transform.GetLocation() + Transform.TransformVector(Collection->GetScrollableBounds().RelativeCenter);
			const FVector Right = Transform.GetUnitAxis(EAxis::Y);
			const FVector Up = Transform.GetUnitAxis(EAxis::Z);
			const FVector Extents = Collection->GetScrollableBounds().Extents;

			// Vertical boundaries.
			{
				const int32 Count = (Collection->GetScrollDirection() == EUxtScrollDirection::UpAndDown ? Collection->GetTiers()
																										: Collection->GetViewableArea()) -
									1;

				const FVector Offset = Right * Collection->GetCellSize().Y;
				FVector Origin = Center - (Offset * 0.5f) + (Offset * (0.5f * Count));

				for (int32 Index = 0; Index < Count; ++Index)
				{
					DrawDashedLine(PDI, Origin + Up * Extents.Z, Origin - Up * Extents.Z, FColor::White, 0.1, SDPG_Foreground);
					Origin -= Offset;
				}
			}

			// Horizontal boundaries.
			{
				const int32 Count = (Collection->GetScrollDirection() == EUxtScrollDirection::UpAndDown ? Collection->GetViewableArea()
																										: Collection->GetTiers()) -
									1;

				const FVector Offset = Up * Collection->GetCellSize().Z;
				FVector Origin = Center - (Offset * 0.5f) + (Offset * (0.5f * Count));

				for (int32 Index = 0; Index < Count; ++Index)
				{
					DrawDashedLine(PDI, Origin + Right * Extents.Y, Origin - Right * Extents.Y, FColor::White, 0.1, SDPG_Foreground);
					Origin -= Offset;
				}
			}
		}
	}
}
