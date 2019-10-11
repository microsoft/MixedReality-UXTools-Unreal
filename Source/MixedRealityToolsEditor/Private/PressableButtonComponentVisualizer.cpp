#include "PressableButtonComponentVisualizer.h"
#include "MixedRealityToolsEditor.h"
#include <SceneManagement.h>


void FPressableButtonComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if (const UPressableButtonComponent* Button = Cast<const UPressableButtonComponent>(Component))
	{
		FMatrix Matrix = Button->GetComponentTransform().ToMatrixNoScale();
		FVector HalfExtents = 0.5f * FVector(Button->MaxPushDistance, Button->Width, Button->Height);

		// Draw movement box, from resting position to maximum push distance
		{
			FBox Box = FBox::BuildAABB(FVector::ForwardVector * HalfExtents.X, HalfExtents);
			DrawWireBox(PDI, Matrix, Box, FLinearColor::Green, SDPG_Foreground, 0.1f);
		}

		// Draw quad at pressed distance
		{
			FVector Vertices[] =
			{
				FVector(Button->PressedDistance, HalfExtents.Y, HalfExtents.Z),
				FVector(Button->PressedDistance, HalfExtents.Y, -HalfExtents.Z),
				FVector(Button->PressedDistance, -HalfExtents.Y, -HalfExtents.Z),
				FVector(Button->PressedDistance, -HalfExtents.Y, HalfExtents.Z)
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