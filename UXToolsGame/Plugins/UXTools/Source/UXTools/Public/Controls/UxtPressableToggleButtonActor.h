// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtPressableButtonActor.h"

#include "UxtPressableToggleButtonActor.generated.h"

class UUxtToggleStateComponent;

/**
 * A derived actor of AUxtPressableButtonActor with a UUxtToggleStateComponent component to track state and visuals for a
 * button can which can be toggled on or off (checked or unchecked).
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API AUxtPressableToggleButtonActor : public AUxtPressableButtonActor
{
	GENERATED_BODY()

public:
	AUxtPressableToggleButtonActor();

	//
	// AActor interface

	/** Ensures the toggle visuals get updated when constructed. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Subscribes to toggle events and sets the initial toggle state. */
	virtual void BeginPlay() override;

	//
	// AUxtPressableButtonActor interface

	/** Adds toggle visuals and components. */
	virtual void ConstructVisuals() override;

	//
	// AUxtPressableToggleButtonActor interface

	/** Alters the toggle visuals when the toggle state changes. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pressable Button")
	virtual void UpdateToggleVisuals();

	/** Gets if the button was toggled on at BeginPlay. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Pressable Button")
	bool IsInitiallyChecked() const { return bIsInitiallyChecked; }

	/** Sets if the button was toggled on at BeginPlay. This method has no function after BeginPlay. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Pressable Button")
	void SetIsInitiallyChecked(bool InitiallyChecked);

	/** Option to remove the toggle plate if it is not needed for this button (for example in derived classes). */
	UFUNCTION(Category = "Uxt Pressable Button")
	void RemoveTogglePlate();

protected:
	/** Button pressed event delegate. */
	virtual void OnButtonPressed(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer) override;

	/** Button released event delegate. */
	virtual void OnButtonReleased(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer) override;

	/** Updates the toggle visuals when the toggles state changes. */
	UFUNCTION(Category = "Uxt Pressable Button")
	virtual void OnButtonToggled(UUxtToggleStateComponent* ToggleState);

	/** Should the button be toggled on or off at BeginPlay? */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pressable Button", BlueprintGetter = "IsInitiallyChecked", BlueprintSetter = "SetIsInitiallyChecked")
	bool bIsInitiallyChecked = false;

	/** Should the button toggle on press or release? */
	UPROPERTY(EditAnywhere, Category = "Uxt Pressable Button")
	bool bToggleOnRelease = false;

	/** Component which keeps track of the toggled state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pressable Button")
	UUxtToggleStateComponent* ToggleStateComponent = nullptr;

	/** Visual component to indicate the toggled state. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pressable Button")
	UUxtBackPlateComponent* TogglePlateComponent = nullptr;
};
