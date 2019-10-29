// Fill out your copyright notice in the Description page of Project Settings.


#include "MixedRealityToolsFunctionLibrary.h"
#include "AudioDevice.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"


bool UMixedRealityToolsFunctionLibrary::IsHandTrackingAvailable()
{
	if (UWindowsMixedRealityHandTrackingFunctionLibrary::SupportsHandTracking())
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			// We take bUseVRPreviewForPlayWorld as an indication that we're running using Holographic Remoting
			UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
			return EdEngine->bUseVRPreviewForPlayWorld;
		}
#endif

		// We assume hand tracking is always available in game mode
		return true;
	}

	return false;
}