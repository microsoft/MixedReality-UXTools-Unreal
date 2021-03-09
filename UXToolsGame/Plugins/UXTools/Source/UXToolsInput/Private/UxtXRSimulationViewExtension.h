// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class UUxtXRSimulationSubsystem;

/**
 * Scene view extension for hiding the player pawn in simulated HMD views.
 */
class UXTOOLSINPUT_API FUxtXRSimulationViewExtension : public FSceneViewExtensionBase
{
public:
	FUxtXRSimulationViewExtension(const FAutoRegister& AutoRegister, UUxtXRSimulationSubsystem* InSimulationSubsystem);

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual bool IsActiveThisFrame(class FViewport* InViewport) const;

private:
	UUxtXRSimulationSubsystem* SimulationSubsystem;
};
