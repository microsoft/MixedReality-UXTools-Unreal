#pragma once

#include "ComponentVisualizer.h"
#include "PressableButtonComponent.h"


class FPressableButtonComponentVisualizer : public FComponentVisualizer
{
private:

	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
};
