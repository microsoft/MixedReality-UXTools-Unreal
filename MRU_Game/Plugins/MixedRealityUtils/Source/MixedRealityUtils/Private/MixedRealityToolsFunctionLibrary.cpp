// Fill out your copyright notice in the Description page of Project Settings.


#include "MixedRealityToolsFunctionLibrary.h"
#include "AudioDevice.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet\GameplayStatics.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"


FTransform UMixedRealityToolsFunctionLibrary::GetHeadPose(const UObject* WorldContextObject)
{
	FRotator rot;
	FVector pos;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(rot, pos);

	if (APlayerCameraManager* Manager = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0))
	{
		pos += Manager->GetTransformComponent()->GetComponentLocation();
	}

	return FTransform(rot, pos);
}

bool UMixedRealityToolsFunctionLibrary::ShouldSimulateHands()
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		// We take bUseVRPreviewForPlayWorld as an indication that we're running using Holographic Remoting
		UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
		return !EdEngine->bUseVRPreviewForPlayWorld;
	}
#endif

	// We assume hand tracking is always available in game mode
	return false;
}