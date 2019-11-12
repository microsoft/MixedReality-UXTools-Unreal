// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MixedRealityToolsFunctionLibrary.generated.h"

/**
 * Library of utility functions for Mixed Reality.
 */
UCLASS()
class MIXEDREALITYUTILS_API UMixedRealityToolsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Returns the world space position and orientation of the head. */
	UFUNCTION(BlueprintPure, Category = "MixedRealityTools", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static FTransform GetHeadPose(const UObject* WorldContextObject);

	/** Returns true if hand tracking is available. Useful to decide whether hand tracking emulation is required. */
	UFUNCTION(BlueprintPure, Category="MixedRealityTools")
	static bool IsHandTrackingAvailable();
};
