// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MixedRealityToolsFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class MIXEDREALITYTOOLS_API UMixedRealityToolsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Returns whether we're currently running in VR */
	UFUNCTION(BlueprintPure, Category="MixedRealityTools")
	static bool IsInVR();
};
