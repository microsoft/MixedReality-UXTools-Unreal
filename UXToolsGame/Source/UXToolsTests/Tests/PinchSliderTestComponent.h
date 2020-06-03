#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"
#include "Controls/UxtPinchSliderComponent.h"

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
	void OnInteractionStarted(UUxtPinchSliderComponent* SliderComponent)
	{
		OnInteractionStartedReceived = true;
	}

	UFUNCTION()
	void OnInteractionEnded(UUxtPinchSliderComponent* SliderComponent)
	{
		OnInteractionEndedReceived = true;
	}

	UFUNCTION()
		void OnFocusEnter(UUxtPinchSliderComponent* SliderComponent)
	{
		OnFocusEnterReceived = true;
	}
	
	UFUNCTION()
		void OnFocusExit(UUxtPinchSliderComponent* SliderComponent)
	{
		OnFocusExitReceived = true;
	}
	UFUNCTION()
		void OnValueUpdated(UUxtPinchSliderComponent* SliderComponent, float value)
	{
		OnValueUpdatedReceived = true;
	}

	bool OnInteractionStartedReceived = false;
	bool OnInteractionEndedReceived = false;
	bool OnFocusEnterReceived = false;
	bool OnFocusExitReceived = false;
	bool OnValueUpdatedReceived = false;


};