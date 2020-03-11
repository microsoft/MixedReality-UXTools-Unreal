// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/UxtFunctionLibrary.h"
#include "AudioDevice.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif


FTransform UUxtFunctionLibrary::GetHeadPose(const UObject* WorldContextObject)
{
	FRotator rot;
	FVector pos;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(rot, pos);

	// Add camera position only when not playing in editor as input simulation already accounts for it in the head pose
	if (!IsInEditor())
	{
		if (APlayerCameraManager* Manager = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0))
		{
			pos += Manager->GetTransformComponent()->GetComponentLocation();
		}
	}

	return FTransform(rot, pos);
}

bool UUxtFunctionLibrary::IsInEditor()
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
		return !EdEngine->bUseVRPreviewForPlayWorld;
	}
#endif
	return false;
}