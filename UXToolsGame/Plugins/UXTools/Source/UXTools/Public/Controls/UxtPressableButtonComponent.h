// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interactions/UxtTouchTarget.h"
#include "Interactions/UxtFarTarget.h"

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

//
// Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUxtButtonBeginFocusDelegate, UUxtPressableButtonComponent*, Button, UObject*, Pointer, bool, bWasAlreadyFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonUpdateFocusDelegate, UUxtPressableButtonComponent*, Button, UObject*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUxtButtonEndFocusDelegate, UUxtPressableButtonComponent*, Button, UObject*, Pointer, bool, bIsStillFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonBeginTouchDelegate, UUxtPressableButtonComponent*, Button, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonUpdateTouchDelegate, UUxtPressableButtonComponent*, Button, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonEndTouchDelegate, UUxtPressableButtonComponent*, Button, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonPressedDelegate, UUxtPressableButtonComponent*, Button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonReleasedDelegate, UUxtPressableButtonComponent*, Button);


/**
 * Component that turns the actor it is attached to into a pressable rectangular button.
 */
UCLASS( ClassGroup = UXTools, meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtPressableButtonComponent : public USceneComponent, public IUxtTouchTarget, public IUxtFarTarget
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

	//
	// Events

	/** Event raised when a pointer starts focusing the button. WasFocused indicates if the button was already focused by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonBeginFocusDelegate OnBeginFocus;

	/** Event raised when a focusing pointer updates. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonUpdateFocusDelegate OnUpdateFocus;

	/** Event raised when a pointer ends focusing the Pressable Button. IsFocused indicates if the Pressable Button is still focused by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonEndFocusDelegate OnEndFocus;

	/** Event raised when a pointer starts touching the Pressable Button. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonBeginTouchDelegate OnBeginTouch;

	/** Event raised while a pointer is touching the Pressable Button. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonUpdateTouchDelegate OnUpdateTouch;

	/** Event raised when a pointer ends touching the Pressable Button. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonEndTouchDelegate OnEndTouch;

	/** Event raised when the button reaches the pressed distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonPressedDelegate OnButtonPressed;

	/** Event raised when the a pressed button reaches the released distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonReleasedDelegate OnButtonReleased;

protected:

	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//
	// IUxtTouchTarget interface

	virtual void OnEnterTouchFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateTouchFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitTouchFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual bool GetClosestTouchPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const override;
	virtual void OnBeginTouch_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateTouch_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndTouch_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual EUxtTouchBehaviour GetTouchBehaviour_Implementation() const override;

	//
	// IUxtFarTarget interface

	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:

	/** Generic handler for enter focus events. */
	void OnEnterFocus(UObject* Pointer);

	/** Generic handler for exit focus events. */
	void OnExitFocus(UObject* Pointer);

	/** Visual representation of the button face. This component's transform will be updated as the button is pressed/released. */
	UPROPERTY(EditAnywhere, DisplayName = "Visuals", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"), Category = "Pressable Button")
	FComponentReference VisualsReference;

	/** Returns the distance a given pointer is pushing the button to. */
	float CalculatePushDistance(const UUxtNearPointerComponent* pointer) const;

	/** Get the current pushed position of the button */
	FVector GetCurrentButtonLocation() const;

	/** Number of pointers currently focusing the button. */
	int NumPointersFocusing = 0;

	/** List of currently touching pointers. */
	TSet<UUxtNearPointerComponent*> TouchPointers;
	
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
