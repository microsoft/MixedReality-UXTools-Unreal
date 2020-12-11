// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtPressableToggleButtonActor.h"

#include "UxtPressableCheckButtonActor.generated.h"

/**
 * A derived actor of AUxtPressableToggleButtonActor which represents the toggle state with a check icon.
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API AUxtPressableCheckButtonActor : public AUxtPressableToggleButtonActor
{
	GENERATED_BODY()

public:
	AUxtPressableCheckButtonActor();

	//
	// AUxtPressableToggleButtonActor interface

	/** Alters the toggle visuals when the toggle state changes. */
	virtual void UpdateToggleVisuals() override;

	//
	// AUxtPressableCheckButtonActor interface

	/** Accessor to the button's unchecked icon brush. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Pressable Button")
	const FUxtIconBrush& GetUncheckedIconBrush() const { return UncheckedIconBrush; }

	/** Applies a new unchecked icon brush. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Pressable Button")
	void SetUncheckedIconBrush(const FUxtIconBrush& Brush);

	/** Accessor to the button's checked icon brush. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Pressable Button")
	const FUxtIconBrush& GetCheckedIconBrush() const { return CheckedIconBrush; }

	/** Applies a new checked icon brush. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Pressable Button")
	void SetCheckedIconBrush(const FUxtIconBrush& Brush);

protected:
	/** Structure which contains properties for the button's icon when unchecked. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pressable Button", BlueprintGetter = "GetUncheckedIconBrush",
		BlueprintSetter = "SetUncheckedIconBrush")
	FUxtIconBrush UncheckedIconBrush;

	/** Structure which contains properties for the button's icon when checked. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pressable Button", BlueprintGetter = "GetCheckedIconBrush", BlueprintSetter = "SetCheckedIconBrush")
	FUxtIconBrush CheckedIconBrush;
};
