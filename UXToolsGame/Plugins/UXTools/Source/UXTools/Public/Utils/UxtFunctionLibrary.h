// Copyright (c) 2020 Microsoft Corporation.
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
UCLASS(ClassGroup = "UXTools")
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

	/**
	 * Returns the scene component the passed component reference is pointing to if there is any - else will return nullptr.
	 *
	 * Replicates FComponentReference::GetComponent() functionality, except it does not use FProperty to detect components.
	 * This allows it to detect components not contained in a blueprint.
	 */
	static USceneComponent* GetSceneComponentFromReference(const FComponentReference& ComponentRef, AActor* Owner);

public:
	/** When true, the methods in this class will use test data. Intended for tests and internal usage only. */
	static bool bUseTestData;

	/** When bUseTestData is true, GetHeadPose will return this transform. */
	static FTransform TestHeadPose;
};
