// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "UxtToggleStateComponent.generated.h"

class UUxtToggleStateComponent;

//
// Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtToggledDelegate, UUxtToggleStateComponent*, ToggleState);

/**
 * Component which holds the state, methods, and delegates responsible for controls with binary states.
 */
UCLASS(ClassGroup = UXTools, HideCategories = (Materials), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtToggleStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Mutates the checked flag and broadcasts events if the state changes.  */
	UFUNCTION(BlueprintSetter, Category = "Toggle State")
	void SetIsChecked(bool IsChecked);

	/** Accessor to the checked flag. */
	UFUNCTION(BlueprintGetter, Category = "Toggle State")
	bool IsChecked() const { return bIsChecked; }

	//
	// Events

	/** Event which broadcasts when the checked state changes. */
	UPROPERTY(BlueprintAssignable, Category = "Toggle State")
	FUxtToggledDelegate OnToggled;

protected:
	/** The current toggled state, true if checked, false is not checked. */
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetIsChecked", BlueprintGetter = "IsChecked", Category = "Toggle State")
	bool bIsChecked = false;
};
