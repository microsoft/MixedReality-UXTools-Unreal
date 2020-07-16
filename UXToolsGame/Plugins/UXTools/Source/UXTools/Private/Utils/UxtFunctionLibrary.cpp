// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Utils/UxtFunctionLibrary.h"
#include "AudioDevice.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif


FTransform UUxtFunctionLibrary::GetHeadPose(UObject* WorldContextObject)
{
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(WorldContextObject, 0);
	if (CameraManager)
	{
		USceneComponent* TransformComp = CameraManager->GetTransformComponent();
		if (TransformComp)
		{
			return TransformComp->GetComponentTransform();
		}
	}
	return FTransform::Identity;
}

bool UUxtFunctionLibrary::IsInEditor()
{
#if WITH_EDITOR
	if (GIsEditor)
	{
		UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
		if (EdEngine->GetPlayInEditorSessionInfo().IsSet())
		{
			return EdEngine->GetPlayInEditorSessionInfo()->OriginalRequestParams.SessionPreviewTypeOverride != EPlaySessionPreviewType::VRPreview;
		}
	}
#endif
	return false;
}

USceneComponent* UUxtFunctionLibrary::GetSceneComponentFromReference(const FComponentReference& ComponentRef, AActor* Owner)
{
	if (ComponentRef.OverrideComponent.IsValid())
	{
		return Cast<USceneComponent>(ComponentRef.OverrideComponent.Get());
	}

	if (AActor* Actor = ComponentRef.OtherActor ? ComponentRef.OtherActor : Owner)
	{
		if (ComponentRef.ComponentProperty != NAME_None)
		{
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component->GetFName() == ComponentRef.ComponentProperty)
				{
					if (USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
					{
						return SceneComponent;
					}
				}
			}
		}
		else if (!ComponentRef.PathToComponent.IsEmpty())
		{
			return FindObject<USceneComponent>(Actor, *ComponentRef.PathToComponent);
		}
		else
		{
			return Actor->GetRootComponent();
		}
	}

	return nullptr;
}
