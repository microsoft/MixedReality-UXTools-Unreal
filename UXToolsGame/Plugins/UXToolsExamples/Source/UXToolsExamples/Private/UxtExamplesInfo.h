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
UCLASS(BlueprintType, Category = "UX Tools Examples")
class UUxtExamplesInfo : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Git Commit information is only set in internal test builds.
	// In all other scenarios this function returns an empty string.
	UFUNCTION(BlueprintCallable, Category = "UXT Examples")
	static FText CommitSHA(const UObject* WorldContext);
};
