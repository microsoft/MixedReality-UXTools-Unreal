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
	FRotator Rotation;
	FVector Position;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(Rotation, Position);

	FTransform TrackingSpaceTransform(Rotation, Position);
	FTransform TrackingToWorld = UHeadMountedDisplayFunctionLibrary::GetTrackingToWorldTransform(WorldContextObject);

	FTransform Result;
	FTransform::Multiply(&Result, &TrackingSpaceTransform, &TrackingToWorld);

	return Result;
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


USceneComponent* UUxtFunctionLibrary::GetSceneComponentFromReference(const FComponentReference& ComponentRef, const AActor* Owner)
{
	if (ComponentRef.ComponentProperty != NAME_None)
	{
		// FComponentReference::GetComponent() doesn't seem to find the component if it's not part of the inherited blueprint.
		const AActor* Actor = ComponentRef.OtherActor ? ComponentRef.OtherActor : Owner;
		const TSet<UActorComponent*>& Components = Actor->GetComponents();

		for (UActorComponent* Component : Components)
		{
			if (Component->GetFName() == ComponentRef.ComponentProperty)
			{
				USceneComponent* SceneComponent = Cast<USceneComponent>(Component);
				if (SceneComponent)
				{
					return SceneComponent;
				}
			}
		}
	}

	return nullptr;
}
