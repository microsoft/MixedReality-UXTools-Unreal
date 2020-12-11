// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Input/UxtPointerComponent.h"
#include "Misc/AutomationTest.h"

#include "PressableButtonTestComponent.generated.h"

class UUxtPressableButtonComponent;

/**
 * Target for button tests that counts button events.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UPressableButtonTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void IncrementPressed(UUxtPressableButtonComponent* ButtonComponent, UUxtPointerComponent* Pointer) { PressedCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void IncrementReleased(UUxtPressableButtonComponent* ButtonComponent, UUxtPointerComponent* Pointer) { ReleasedCount++; }

	int PressedCount = 0;
	int ReleasedCount = 0;
};
