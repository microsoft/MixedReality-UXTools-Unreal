// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtPressableButtonComponentVisualizer.h"

#include "UXToolsEditor.h"

#include "Utils/UxtMathUtilsFunctionLibrary.h"

#include <SceneManagement.h>

namespace
{
	void DrawQuad(
		FPrimitiveDrawInterface* PDI, float Width, float Height, float PressedDistance, const FMatrix& Transform, const FLinearColor& Color,
		bool bDashed = false)
	{
		FVector Vertices[] = {
			FVector(-PressedDistance, Width, Height), FVector(-PressedDistance, Width, -Height), FVector(-PressedDistance, -Width, -Height),
			FVector(-PressedDistance, -Width, Height)};

		for (int i = 0; i < 4; ++i)
		{
			FVector Start = Transform.TransformPosition(Vertices[i]);
			FVector End = Transform.TransformPosition(Vertices[(i + 1) % 4]);
			if (bDashed)
			{
				DrawDashedLine(PDI, Start, End, Color, 0.1, SDPG_Foreground);
			}
			else
			{
				PDI->DrawLine(Start, End, Color, SDPG_Foreground, 0.05f);
			}
		}
	}
} // namespace

void FUxtPressableButtonComponentVisualizer::DrawVisualization(
	const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UUxtPressableButtonComponent* Button = Cast<const UUxtPressableButtonComponent>(Component))
	{
		if (USceneComponent* Visuals = Button->GetVisuals())
		{
			FTransform LocalToTarget = Visuals->GetComponentTransform() * Button->GetComponentTransform().Inverse();
			FBox Bounds = UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(
							  Visuals, LocalToTarget, UUxtPressableButtonComponent::VisualBoundsFilter)
							  .GetBox();

			FTransform ToFrontFace = FTransform(FVector(Bounds.Max.X, 0, 0));
			FMatrix FrontFaceMatrix = (ToFrontFace * Button->GetComponentTransform()).ToMatrixNoScale();

			FVector Extents = Bounds.GetExtent();
			Extents *= Button->GetComponentTransform().GetScale3D();

			// Rest position
			const float MaxPushDistance = Button->GetScaleAdjustedMaxPushDistance();
			const float FrontFaceMargin = Button->GetFrontFaceCollisionFraction() * MaxPushDistance;
			DrawQuad(PDI, Extents.Y, Extents.Z, -FrontFaceMargin, FrontFaceMatrix, FLinearColor::White);

			// Maximum push distance
			FLinearColor DarkGray(.25f, .25f, .25f);
			DrawQuad(PDI, Extents.Y, Extents.Z, MaxPushDistance - FrontFaceMargin, FrontFaceMatrix, DarkGray);

			// Pressed distance
			const float PressedDistance = MaxPushDistance * Button->PressedFraction;
			FLinearColor LightGray(.75f, .75f, .75f);
			DrawQuad(PDI, Extents.Y, Extents.Z, PressedDistance - FrontFaceMargin, FrontFaceMatrix, LightGray, true);
		}
	}
}
