// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Input/UxtFarPointerComponent.h"

#include "FarPointerListenerComponent.generated.h"

/** Listens to and counts enable/disable events raised by a UxtFarPointerComponent. */
UCLASS(ClassGroup = "UXToolsTests")
class UXTOOLSTESTS_API UFarPointerListenerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer) { NumEnabled++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer) { NumDisabled++; }

public:
	int NumEnabled = 0;
	int NumDisabled = 0;
};
