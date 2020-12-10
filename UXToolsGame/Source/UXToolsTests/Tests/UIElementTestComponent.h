// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"

#include "UIElementTestComponent.generated.h"

class UUxtUIElementComponent;

/**
 * Component to track show / hide events for UxtUIElementComponent tests.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UUIElementTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void OnShowElement(UUxtUIElementComponent* UIElement) { ShowCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnHideElement(UUxtUIElementComponent* UIElement, bool bShouldAffectLayout) { HideCount++; }

public:
	int ShowCount = 0;
	int HideCount = 0;
};
