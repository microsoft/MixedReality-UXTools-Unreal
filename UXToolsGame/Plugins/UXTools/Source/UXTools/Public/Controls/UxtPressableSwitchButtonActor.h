// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtPressableRadioButtonActor.h"

#include "UxtPressableSwitchButtonActor.generated.h"

/**
 * A derived actor of AUxtPressableRadioButtonActor which represents the toggle state with a switch that animates
 * left and right.
 */
UCLASS(ClassGroup = UXTools)
class UXTOOLS_API AUxtPressableSwitchButtonActor : public AUxtPressableRadioButtonActor
{
	GENERATED_BODY()

public:
	AUxtPressableSwitchButtonActor();

	//
	// AUxtPressableToggleButtonActor interface

	/** Alters the toggle visuals when the toggle state changes. */
	virtual void UpdateToggleVisuals() override;

	//
	// AUxtPressableSwitchButtonActor interface

	/** Accessor to the button's switched off icon brush. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	const FUxtIconBrush& GetSwitchedOffIconBrush() const { return SwitchedOffIconBrush; }

	/** Applies a new switched off icon brush. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetSwitchedOffIconBrush(const FUxtIconBrush& Brush);

	/** Accessor to the button's switched on icon brush. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	const FUxtIconBrush& GetSwitchedOnIconBrush() const { return SwitchedOnIconBrush; }

	/** Applies a new switched on icon brush. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetSwitchedOnIconBrush(const FUxtIconBrush& Brush);

protected:
	/** Structure which contains properties for the button's icon when switched off. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = "GetSwitchedOffIconBrush", BlueprintSetter = "SetSwitchedOffIconBrush", Category = "SwitchButton")
	FUxtIconBrush SwitchedOffIconBrush;

	/** Structure which contains properties for the button's icon when switched on. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = "GetSwitchedOnIconBrush", BlueprintSetter = "SetSwitchedOnIconBrush", Category = "SwitchButton")
	FUxtIconBrush SwitchedOnIconBrush;
};
