// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

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
	static FTransform GetHeadPose(UObject* WorldContextObject);

	/** Returns true if we are running in editor (not game mode or VR preview). */
	UFUNCTION(BlueprintPure, Category = "UXTools")
	static bool IsInEditor();

	/** Converts a Unicode code point as hex into the corresponding UTF-16 FString representation. Returns true when the conversion is successful.*/
	UFUNCTION(BlueprintPure, Category = "UXTools")
	static bool HexCodePointToFString(const FString& Input, FString& Output);
};

