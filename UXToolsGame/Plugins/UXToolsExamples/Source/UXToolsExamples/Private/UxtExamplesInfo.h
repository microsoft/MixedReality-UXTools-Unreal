// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "UxtExamplesInfo.generated.h"

class FText;

/**
 * Internal: provide build metadata in packaged game at runtime.
 */
UCLASS(BlueprintType, Category = "UXToolsExamples")
class UUxtExamplesInfo : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Git Commit information is only set in internal test builds.
	// In all other scenarios this function returns an empty string.
	UFUNCTION(BlueprintCallable, Category = "UXTools|Examples Info")
	static FText CommitSHA(const UObject* WorldContext);
};
