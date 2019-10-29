// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MixedRealityToolsFunctionLibrary.generated.h"

/**
 * Library of utility functions for Mixed Reality.
 */
UCLASS()
class MIXEDREALITYTOOLS_API UMixedRealityToolsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Returns true if hand tracking is available. Useful to decide whether hand tracking emulation is required. */
	UFUNCTION(BlueprintPure, Category="MixedRealityTools")
	static bool IsHandTrackingAvailable();
};
