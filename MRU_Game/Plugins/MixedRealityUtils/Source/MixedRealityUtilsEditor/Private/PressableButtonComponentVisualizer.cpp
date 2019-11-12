#include "PressableButtonComponentVisualizer.h"
#include "MixedRealityUtilsEditor.h"
#include <SceneManagement.h>


void FPressableButtonComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UPressableButtonComponent* Button = Cast<const UPressableButtonComponent>(Component))
	{
		FVector Extents = Button->GetComponentScale() * Button->Extents;
		FMatrix Matrix = Button->GetComponentTransform().ToMatrixNoScale();
		
		// Draw movement box, from resting position to maximum push distance
		{
			FBox Box = FBox::BuildAABB(FVector::ForwardVector * Extents.X, Extents);
			DrawWireBox(PDI, Matrix, Box, FLinearColor::Green, SDPG_Foreground, 0.1f);
		}

		// Draw quad at pressed distance
		{
			float PressedDistance = Extents.X * Button->PressedFraction * 2;
			FVector Vertices[] =
			{
				FVector(PressedDistance, Extents.Y, Extents.Z),
				FVector(PressedDistance, Extents.Y, -Extents.Z),
				FVector(PressedDistance, -Extents.Y, -Extents.Z),
				FVector(PressedDistance, -Extents.Y, Extents.Z)
			};

			for (int i = 0; i < 4; ++i)
			{
				FVector Start = Matrix.TransformPosition(Vertices[i]);
				FVector End = Matrix.TransformPosition(Vertices[(i + 1) % 4]);
				PDI->DrawLine(Start, End, FLinearColor::Blue, SDPG_Foreground, 0.1f);
			}
		}
	}
}