#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Controls/UxtPinchSliderComponent.h"
#include "Misc/AutomationTest.h"

#include "PinchSliderTestComponent.generated.h"

class UUxtPinchSliderComponent;

/**
 * Target for button tests that counts button events.
 */
UCLASS()
class UPinchSliderTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnInteractionStarted(UUxtPinchSliderComponent* SliderComponent, UUxtPointerComponent* Pointer)
	{
		OnInteractionStartedReceived = true;
	}

	UFUNCTION()
	void OnInteractionEnded(UUxtPinchSliderComponent* SliderComponent, UUxtPointerComponent* Pointer) { OnInteractionEndedReceived = true; }

	UFUNCTION()
	void OnFocusEnter(UUxtPinchSliderComponent* SliderComponent, UUxtPointerComponent* Pointer, bool bWasAlreadyFocused)
	{
		OnFocusEnterReceived = true;
	}

	UFUNCTION()
	void OnFocusExit(UUxtPinchSliderComponent* SliderComponent, UUxtPointerComponent* Pointer, bool bIsStillFocused)
	{
		OnFocusExitReceived = true;
	}
	UFUNCTION()
	void OnValueUpdated(UUxtPinchSliderComponent* SliderComponent, float value) { OnValueUpdatedReceived = true; }

	bool OnInteractionStartedReceived = false;
	bool OnInteractionEndedReceived = false;
	bool OnFocusEnterReceived = false;
	bool OnFocusExitReceived = false;
	bool OnValueUpdatedReceived = false;
};