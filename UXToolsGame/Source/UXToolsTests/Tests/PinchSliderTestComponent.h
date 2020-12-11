// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

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
UCLASS(ClassGroup = "UXToolsTests")
class UPinchSliderTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	//
	// Test component interface.

	void Reset()
	{
		OnUpdateStateReceived = false;
		OnBeginFocusReceived = false;
		OnUpdateFocusReceived = false;
		OnEndFocusReceived = false;
		OnBeginGrabReceived = false;
		OnUpdateValueReceived = false;
		OnEndGrabReceived = false;
		OnEnableReceived = false;
		OnDisableReceived = false;
	}

	//
	// UxtPinchSlider event callbacks.

	UFUNCTION(Category = "UXToolsTests")
	void OnUpdateState(UUxtPinchSliderComponent* Slider, EUxtSliderState NewState) { OnUpdateStateReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnBeginFocus(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer, bool bWasAlreadyFocused)
	{
		OnBeginFocusReceived = true;
	}

	UFUNCTION(Category = "UXToolsTests")
	void OnUpdateFocus(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer) { OnUpdateFocusReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnEndFocus(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer, bool bIsStillFocused) { OnEndFocusReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnBeginGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer) { OnBeginGrabReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnUpdateValue(UUxtPinchSliderComponent* Slider, float NewValue) { OnUpdateValueReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnEndGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer) { OnEndGrabReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnEnable(UUxtPinchSliderComponent* Slider) { OnEnableReceived = true; }

	UFUNCTION(Category = "UXToolsTests")
	void OnDisable(UUxtPinchSliderComponent* Slider) { OnDisableReceived = true; }

public:
	//
	// Flags to indicate which events have been received.

	bool OnUpdateStateReceived = false;
	bool OnBeginFocusReceived = false;
	bool OnUpdateFocusReceived = false;
	bool OnEndFocusReceived = false;
	bool OnBeginGrabReceived = false;
	bool OnUpdateValueReceived = false;
	bool OnEndGrabReceived = false;
	bool OnEnableReceived = false;
	bool OnDisableReceived = false;
};
