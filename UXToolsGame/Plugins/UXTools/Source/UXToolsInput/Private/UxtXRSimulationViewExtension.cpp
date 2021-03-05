// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtXRSimulationViewExtension.h"

#include "UxtDefaultHandTrackerSubsystem.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

FUxtXRSimulationViewExtension::FUxtXRSimulationViewExtension(
	const FAutoRegister& AutoRegister, UUxtDefaultHandTrackerSubsystem* InHandTrackerSubsystem)
	: FSceneViewExtensionBase(AutoRegister)
	, HandTrackerSubsystem(InHandTrackerSubsystem)
{
}

void FUxtXRSimulationViewExtension::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FUxtXRSimulationViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	UWorld* World = InViewFamily.Scene->GetWorld();
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			for (const UActorComponent* Component : Pawn->GetComponents())
			{
				if (const UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
				{
					InView.HiddenPrimitives.Add(PrimComp->ComponentId);
				}
			}
		}
	}
}

void FUxtXRSimulationViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
}

void FUxtXRSimulationViewExtension::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());
}

void FUxtXRSimulationViewExtension::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	check(IsInRenderingThread());
}

bool FUxtXRSimulationViewExtension::IsActiveThisFrame(class FViewport* InViewport) const
{
	return HandTrackerSubsystem->IsSimulationEnabled();
}
