// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/UxtFunctionLibrary.h"
#include "AudioDevice.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet\GameplayStatics.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"


FTransform UUxtFunctionLibrary::GetHeadPose(const UObject* WorldContextObject)
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
