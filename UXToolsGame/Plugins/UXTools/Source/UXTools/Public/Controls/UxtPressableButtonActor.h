// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtBasePressableButtonActor.h"
#include "Controls/UxtButtonBrush.h"
#include "Controls/UxtIconBrush.h"

#include "UxtPressableButtonActor.generated.h"

class UUxtBackPlateComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UAudioComponent;

/**
 * The default pressable button actor which programmatically builds an actor hierarchy with a back plate, front plate,
 * icon, and label. All button properties within this class are reactive at edit and runtime. This actor also contains
 * behaviors to support icon focus animation and sound playback. This class is extensible to support derived button types.
 */
UCLASS(ClassGroup = UXTools)
class UXTOOLS_API AUxtPressableButtonActor : public AUxtBasePressableButtonActor
{
	GENERATED_BODY()

public:
	AUxtPressableButtonActor();

	//
	// AActor interface

	/** Creates (and initializes) the button hierarchy when properties are changed. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Conditional tick method which occurs when a button needs to animate. */
	virtual void Tick(float DeltaTime) override;

	//
	// AUxtPressableButtonActor interface

	/** Creates (and initializes) the button's visual hierarchy. */
	UFUNCTION(BlueprintCallable, Category = "Button")
	virtual void ConstructVisuals();

	/** Creates (and initializes) the button's icon hierarchy. */
	UFUNCTION(BlueprintCallable, Category = "Button")
	virtual void ConstructIcon();

	/** Creates (and initializes) the button's label hierarchy. */
	UFUNCTION(BlueprintCallable, Category = "Button")
	virtual void ConstructLabel();

	/** Starts the pulse animation for a given pointer. */
	UFUNCTION(BlueprintCallable, Category = "Button")
	bool BeginPulse(const UUxtPointerComponent* Pointer);

	/** Returns true if a pulse is currently animating. */
	UFUNCTION(BlueprintPure, Category = "Button")
	bool IsPulsing() const { return PulseTimer >= 0; }

	/** Accessor to the button size in millimeters. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	FVector GetMillimeterSize() const { return MillimeterSize; }

	/** Sets the button size in millimeters. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetMillimeterSize(FVector Size);

	/** Accessor to the button size in default units. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	FVector GetSize() const { return MillimeterSize * 0.1f; }

	/** Sets the button size in default units. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetSize(FVector Size);

	/** Accessor to if the button is plated. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	bool IsPlated() const { return bIsPlated; }

	/** Enables or disabled the button back plate. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetIsPlated(bool IsPlated);

	/** Accessor to the button's icon brush. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	const FUxtIconBrush& GetIconBrush() const { return IconBrush; }

	/** Applies a new icon brush. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetIconBrush(const FUxtIconBrush& Brush);

	/** Accessor to the button's label. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	const FText& GetLabel() const { return Label; }

	/** Applies a new label. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetLabel(const FText& NewLabel);

	/** Accessor to the button's label text brush. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	const FUxtTextBrush& GetLabelTextBrush() const { return LabelTextBrush; }

	/** Applies a new label text brush. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetLabelTextBrush(const FUxtTextBrush& Brush);

	/** Accessor to the button's button brush. */
	UFUNCTION(BlueprintGetter, Category = "Button")
	const FUxtButtonBrush& GetButtonBrush() const { return ButtonBrush; }

	/** Applies a new button brush. */
	UFUNCTION(BlueprintSetter, Category = "Button")
	void SetButtonBrush(const FUxtButtonBrush& Brush);

protected:
	/** Method which is invoked when the button is pressed. */
	UFUNCTION(Category = "Button")
	virtual void OnButtonPressed(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer);

	/** Method which is invoked when the button is released. */
	UFUNCTION(Category = "Button")
	virtual void OnButtonReleased(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer);

	/** Method which is invoked when the button is focused on. */
	UFUNCTION(Category = "Button")
	virtual void OnBeginFocus(UUxtPressableButtonComponent* Button, UUxtPointerComponent* Pointer, bool WasAlreadyFocused);

	/** Method which is invoked when the button is enabled. */
	UFUNCTION(Category = "Button")
	virtual void OnButtonEnabled(UUxtPressableButtonComponent* Button);

	/** Method which is invoked when the button is disabled. */
	UFUNCTION(Category = "Button")
	virtual void OnButtonDisabled(UUxtPressableButtonComponent* Button);

	/** Method to update the pulse animation and behavior. Returns true when the animation is complete. */
	virtual bool AnimatePulse(float DeltaTime);

	/** Method to update the focus animation and behavior. Returns true when the animation is complete. */
	virtual bool AnimateFocus(float DeltaTime);

	/** Utility method to allocate and add a scene component to the button. */
	template <class T>
	T* CreateAndAttachComponent(FName Name, USceneComponent* Parent)
	{
		T* Component = CreateDefaultSubobject<T>(Name);
		Component->SetupAttachment(Parent);
		return Component;
	}

	/** The millimeter size of the button which dynamically resizes components within the button. This will preserve the actor scale. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetMillimeterSize", BlueprintSetter = "SetMillimeterSize", Category = "Button")
	FVector MillimeterSize = FVector(16, 32, 32);

	/** True if the button should display a back plate. Collections of buttons should share a common back plate. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "IsPlated", BlueprintSetter = "SetIsPlated", Category = "Button")
	bool bIsPlated = true;

	/** Structure which contains properties for the button's icon. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = "GetIconBrush", BlueprintSetter = "SetIconBrush", Category = "Button",
		meta = (EditCondition = "bCanEditIconBrush"))
	FUxtIconBrush IconBrush;

	/** Localizable text for the label. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetLabel", BlueprintSetter = "SetLabel", Category = "Button")
	FText Label = NSLOCTEXT("PressableButtonActor", "LabelDefault", "16x32x32mm");

	/** Text settings for the label. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetLabelTextBrush", BlueprintSetter = "SetLabelTextBrush", Category = "Button")
	FUxtTextBrush LabelTextBrush;

	/** Structure which contains properties for the button's appearance and behavior. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetButtonBrush", BlueprintSetter = "SetButtonBrush", Category = "Button")
	FUxtButtonBrush ButtonBrush;

	/** Pivot component to support back plate compression visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	USceneComponent* BackPlatePivotComponent = nullptr;

	/** Back plate mesh component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	UUxtBackPlateComponent* BackPlateMeshComponent = nullptr;

	/** Pivot component to support front plate compression visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	USceneComponent* FrontPlatePivotComponent = nullptr;

	/** The center of mass pivot of the front plate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	USceneComponent* FrontPlateCenterComponent = nullptr;

	/** Front plate mesh component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	UStaticMeshComponent* FrontPlateMeshComponent = nullptr;

	/** Icon text component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	UTextRenderComponent* IconComponent = nullptr;

	/** Label text component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	UTextRenderComponent* LabelComponent = nullptr;

	/** Audio playback component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Button")
	UAudioComponent* AudioComponent = nullptr;

	/** The current animation time of the pulse animation. */
	float PulseTimer = -1;

	/** The current animation time of the pulse fade out animation. */
	float PulseFadeTimer = -1;

	/** Handle to the original (unaltered) material before pulsing. */
	UPROPERTY(Transient)
	UMaterialInterface* PrePulseMaterial = nullptr;

	/** Handle to any dynamic material the pulse instantiates due to material parameter changes. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* PulseMaterialInstance = nullptr;

	/** The active material based on which pointer triggered the pulse. */
	uint32 MaterialIndex = 0;

	/** The current animation time of the focus animation. */
	float FocusTimer = 0;

	/** Allows derived classes to control if the icon brush can be edited. */
	UPROPERTY(EditDefaultsOnly, Category = "Button")
	bool bCanEditIconBrush = true;
};
