// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"

#include "TooltipEventListener.generated.h"

/**
 * Class to receive the events from the unittest.
 */
UCLASS()
class UTooltipEventListener : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnShow() { ShowCount++; }

	UFUNCTION()
	void OnHide() { HideCount++; }

	int ShowCount = 0;
	int HideCount = 0;
};
