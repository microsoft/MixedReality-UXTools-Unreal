// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "TapToPlaceTestComponent.generated.h"

class UUxtFarPointerComponent;
class UUxtTapToPlaceComponent;

/**
 * Target for tap to place tests that counts tap to place events.
 */
UCLASS()
class UTapToPlaceTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnPlacementStarted(UUxtTapToPlaceComponent* TapToPlaceComponent) { OnPlacementStartedReceived = true; }

	UFUNCTION()
	void OnPlacementEnded(UUxtTapToPlaceComponent* TapToPlaceComponent) { OnPlacementEndedReceived = true; }

	UFUNCTION()
	void OnFocusEnter(UUxtTapToPlaceComponent* TapToPlaceComponent, UUxtFarPointerComponent* Pointer, bool bWasAlreadyFocused)
	{
		OnFocusEnterReceived = true;
	}

	UFUNCTION()
	void OnFocusUpdated(UUxtTapToPlaceComponent* TapToPlaceComponent, UUxtFarPointerComponent* Pointer) { OnFocusUpdatedReceived = true; }

	UFUNCTION()
	void OnFocusExit(UUxtTapToPlaceComponent* TapToPlaceComponent, UUxtFarPointerComponent* Pointer, bool bIsStillFocused)
	{
		OnFocusExitReceived = true;
	}

	bool OnPlacementStartedReceived = false;
	bool OnPlacementEndedReceived = false;
	bool OnFocusEnterReceived = false;
	bool OnFocusUpdatedReceived = false;
	bool OnFocusExitReceived = false;
};
