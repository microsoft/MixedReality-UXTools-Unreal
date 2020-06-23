// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UxtFunctionLibrary.generated.h"

struct FComponentReference;
class AActor;
/**
 * Library of utility functions for UX Tools.
 */
UCLASS()
class UXTOOLS_API UUxtFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Returns the world space position and orientation of the head. */
	UFUNCTION(BlueprintPure, Category = "UXTools", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FTransform GetHeadPose(UObject* WorldContextObject);

	/** Returns true if we are running in editor (not game mode or VR preview). */
	UFUNCTION(BlueprintPure, Category = "UXTools")
	static bool IsInEditor();


	/** Returns the scene component the passed component reference is pointing to if there is any - else will return nullptr. */
	static USceneComponent* GetSceneComponentFromReference(const FComponentReference& ComponentRef, const AActor* Owner);
};

