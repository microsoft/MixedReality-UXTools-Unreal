// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"

#include "TouchableVolumeTestComponent.generated.h"

class UUxtTouchableVolumeComponent;

/**
 * Target for touchable volume tests that counts events.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UTouchableVolumeTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void OnBeginFocus(UUxtTouchableVolumeComponent* TargetComponent, UUxtPointerComponent* Pointer, bool bWasAlreadyFocused)
	{
		BeginFocusCount++;
	}

	UFUNCTION(Category = "UXToolsTests")
	void OnEndFocus(UUxtTouchableVolumeComponent* TargetComponent, UUxtPointerComponent* Pointer, bool bIsStillFocused) { EndFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnBeginPoke(UUxtTouchableVolumeComponent* TargetComponent, UUxtPointerComponent* Pointer) { BeginPokeCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnEndPoke(UUxtTouchableVolumeComponent* TargetComponent, UUxtPointerComponent* Pointer) { EndPokeCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnDisable(UUxtTouchableVolumeComponent* TargetComponent) { DisableCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnEnable(UUxtTouchableVolumeComponent* TargetComponent) { EnableCount++; }

	int BeginFocusCount = 0;
	int EndFocusCount = 0;
	int BeginPokeCount = 0;
	int EndPokeCount = 0;
	int DisableCount = 0;
	int EnableCount = 0;
};
