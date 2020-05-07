// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"

#include "PressableButtonTestComponent.generated.h"

class UUxtPressableButtonComponent;

/**
 * Target for button tests that counts button events.
 */
UCLASS()
class UPressableButtonTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void IncrementPressed(UUxtPressableButtonComponent* ButtonComponent)
	{
		PressedCount++;
	}

	UFUNCTION()
	void IncrementReleased(UUxtPressableButtonComponent* ButtonComponent)
	{
		ReleasedCount++;
	}

	int PressedCount = 0;
	int ReleasedCount = 0;
};
