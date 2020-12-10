// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtPressableToggleButtonActor.h"

#include "UxtPressableRadioButtonActor.generated.h"

/**
 * A derived actor of AUxtPressableToggleButtonActor which represents the toggle state with a circle icon. Radio buttons
 * are normally used in groups alongside the UUxtToggleGroupComponent.
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API AUxtPressableRadioButtonActor : public AUxtPressableToggleButtonActor
{
	GENERATED_BODY()

public:
	AUxtPressableRadioButtonActor();

	//
	// AUxtPressableButtonActor interface

	/** Adds toggle visuals. */
	virtual void ConstructIcon() override;

	//
	// AUxtPressableToggleButtonActor interface

	/** Alters the toggle visuals when the toggle state changes. */
	virtual void UpdateToggleVisuals() override;

protected:
	/** Displays the radio button's center icon which gets toggled on and off. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pressable Button")
	UTextRenderComponent* CenterIconComponent = nullptr;
};
