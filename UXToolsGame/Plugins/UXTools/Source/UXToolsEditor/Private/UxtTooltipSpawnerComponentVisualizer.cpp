// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTooltipSpawnerComponentVisualizer.h"

#include "SceneManagement.h"
#include "UXToolsEditor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Tooltips/UxtTooltipActor.h"
#include "Tooltips/UxtTooltipSpawnerComponent.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

void FUxtTooltipSpawnerComponentVisualizer::DrawVisualization(
	const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UUxtTooltipSpawnerComponent* TooltipSpawner = Cast<const UUxtTooltipSpawnerComponent>(Component);
	if (TooltipSpawner == nullptr)
		return;

	const auto LineColor = FLinearColor::White;

	AActor* Owner = TooltipSpawner->GetOwner();
	const USceneComponent* PivotComponent = Cast<USceneComponent>(TooltipSpawner->Pivot.GetComponent(Owner));
	const FVector Offset = PivotComponent ? PivotComponent->GetRelativeLocation() * Owner->GetActorScale3D() : FVector::ZeroVector;
	const FVector LineStart = TooltipSpawner->GetOwner()->GetActorLocation();
	const FVector LineEnd = LineStart + Offset;
	const float LineThickness = 0.05f;
	PDI->DrawLine(LineStart, LineEnd, LineColor, SDPG_Foreground, LineThickness);

	constexpr float Width = 10.0f;
	constexpr float Height = 10.0f;
	constexpr float Depth = 0.0f;
	FTransform Transform = TooltipSpawner->GetOwner()->GetActorTransform();
	Transform.SetLocation(Transform.GetLocation() + Offset);

	const FVector Vertices[] = {
		FVector(-Depth, Width, Height), FVector(-Depth, Width, -Height), FVector(-Depth, -Width, -Height), FVector(-Depth, -Width, Height)};

	for (int i = 0; i < 4; ++i)
	{
		// No scale because we don't want the scale of the target to affect the preview.
		const FVector Start = Transform.TransformPositionNoScale(Vertices[i]);
		const FVector End = Transform.TransformPositionNoScale(Vertices[(i + 1) % 4]);
		PDI->DrawLine(Start, End, LineColor, SDPG_Foreground, LineThickness);
	}
}
