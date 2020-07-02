// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"

#include "UIElementTestComponent.generated.h"

class UUxtUIElement;

/**
 * Component to track activate / deactivate events for UxtUIElement tests.
 */
UCLASS()
class UUIElementTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void OnElementActivated(UUxtUIElement* UIElement)
	{
		OnElementActivatedCount++;
	}

	UFUNCTION()
	void OnElementDeactivated(UUxtUIElement* UIElement)
	{
		OnElementDeactivatedCount++;
	}

public:

	int OnElementActivatedCount = 0;
	int OnElementDeactivatedCount = 0;
};
