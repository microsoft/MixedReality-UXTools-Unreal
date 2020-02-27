// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UxtFunctionLibrary.generated.h"

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
	static FTransform GetHeadPose(const UObject* WorldContextObject);

	/** Returns true if we are running in editor (not game mode or VR preview). */
	UFUNCTION(BlueprintPure, Category = "UXTools")
	static bool IsInEditor();
};
