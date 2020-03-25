// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Interactions/UxtTouchTargetComponent.h"

#include "UxtPressableButtonComponent.generated.h"

namespace Microsoft
{
    namespace MixedReality
    {
        namespace UX
        {
            class PressableButton;
        }
    }
}

class UUxtPressableButtonComponent;
class UUxtFarPointerComponent;
class UBoxComponent;
class UShapeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonPressedDelegate, UUxtPressableButtonComponent*, Button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonReleasedDelegate, UUxtPressableButtonComponent*, Button);


/**
 * Component that turns the actor it is attached to into a pressable rectangular button.
 */
UCLASS( ClassGroup = UXTools, meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtPressableButtonComponent : public UUxtTouchTargetComponent
{
	GENERATED_BODY()

public:	

	UUxtPressableButtonComponent();

	/** Get scene component used for the moving visuals */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	USceneComponent* GetVisuals() const;

	/** Set scene component to be used for the moving visuals */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	void SetVisuals(USceneComponent* Visuals);

	/** Get the current pressed state of the button */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	bool IsPressed() const;

	/** Get the current woldspace extents of the touchable button surface */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	FVector2D GetButtonExtents() const;

	/** The maximum distance the button can be pushed */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	float GetScaleAdjustedMaxPushDistance() const;

protected:

    //
    // UActorComponent interface

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//
	// IUxtTouchTarget interface

	virtual void OnExitTouchFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual EUxtTouchBehaviour GetTouchBehaviour_Implementation() const override;
	
	virtual void OnEndTouch_Implementation(UUxtNearPointerComponent* Pointer) override;
	
	//
	// IUxtFarTarget interface

	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent) override;

public:

	/** Collision profile used by the button collider */
	UPROPERTY(EditAnywhere, Category = "Pressable Button")
	FName CollisionProfile = TEXT("UI");

	/** The maximum distance the button can be pushed */
	UPROPERTY(EditAnywhere, Category = "Pressable Button")
	float MaxPushDistance;

	/** Fraction of the maximum travel distance at which the button will raise the pressed event. */
    UPROPERTY(EditAnywhere, Category = "Pressable Button")
    float PressedFraction;

	/** Fraction of the maximum travel distance at which a pressed button will raise the released event. */
    UPROPERTY(EditAnywhere, Category = "Pressable Button")
    float ReleasedFraction;

	/** Button movement speed while recovering */
    UPROPERTY(EditAnywhere, Category = "Pressable Button")
    float RecoverySpeed;

	/** Event raised when the button reaches the pressed distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonPressedDelegate OnButtonPressed;

	/** Event raised when the a pressed button reaches the released distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonReleasedDelegate OnButtonReleased;

private:

	FVector GetVisualsRestPosition() const;

	/** Visual representation of the button face. This component's transform will be updated as the button is pressed/released. */
	UPROPERTY(EditAnywhere, DisplayName = "Visuals", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"), Category = "Pressable Button")
	FComponentReference VisualsReference;

	/** Returns the distance a given pointer is pushing the button to. */
	float CalculatePushDistance(const UUxtNearPointerComponent* pointer) const;

	/** Get the current pushed position of the button */
	FVector GetCurrentButtonLocation() const;
	
	/** Far pointer currently pressing the button if any */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;


	/** Collision volume used for determining touch events */
	UBoxComponent* BoxComponent;

	/** Visuals offset in this component's space */
	FVector VisualsOffsetLocal;

	/** Visuals offset in this component's space */
	FVector ColliderOffsetLocal;

	/** True if the button is currently pressed */
	bool bIsPressed = false;

	/** Position of the button while not being touched by any pointer */
	FVector RestPosition;

	/** The distance at which the button will fire a pressed event */
	float PressedDistance;

	/** The distance at which the button will fire a released event */
	float ReleasedDistance;

	/** The current pushed distance of from touching pointers */
	float CurrentPushDistance;
};
