// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Input/UxtFarPointerComponent.h"
#include "FarPointerListener.generated.h"

/** Listens to and counts enable/disable events raised by a UxtFarPointerComponent. */
UCLASS()
class UXTOOLSTESTS_API UFarPointerListener : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer)
	{
		NumEnabled++;
	}

	UFUNCTION()
	void OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer)
	{
		NumDisabled++;
	}

public:

	int NumEnabled = 0;
	int NumDisabled = 0;
};

