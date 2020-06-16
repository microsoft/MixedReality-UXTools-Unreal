// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Input/UxtPointerComponent.h"
#include "Interactions/UxtPokeTarget.h"
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

UENUM(BlueprintType)
enum class EUxtPushBehavior : uint8
{
	/** When pushed the button visuals translate */
	Translate,
	/** When pushed the button visuals compress (scale) */
	Compress
};

UENUM(BlueprintType)
enum class EUxtButtonState : uint8
{
	/** Default state, not pressed or disabled */
	Default,
	/** Button is disabled */
	Disabled,
	/** Button is focused */
	Focused,
	/** Button is contacted */
	Contacted,
	/** Button is pressed */
	Pressed
};

//
// Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUxtButtonBeginFocusDelegate, UUxtPressableButtonComponent*, Button, UUxtPointerComponent*, Pointer, bool, bWasAlreadyFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonUpdateFocusDelegate, UUxtPressableButtonComponent*, Button, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUxtButtonEndFocusDelegate, UUxtPressableButtonComponent*, Button, UUxtPointerComponent*, Pointer, bool, bIsStillFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonBeginPokeDelegate, UUxtPressableButtonComponent*, Button, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonUpdatePokeDelegate, UUxtPressableButtonComponent*, Button, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonEndPokeDelegate, UUxtPressableButtonComponent*, Button, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonPressedDelegate, UUxtPressableButtonComponent*, Button, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtButtonReleasedDelegate, UUxtPressableButtonComponent*, Button, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonEnabledDelegate, UUxtPressableButtonComponent*, Button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtButtonDisabledDelegate, UUxtPressableButtonComponent*, Button);


/**
 * Component that turns the actor it is attached to into a pressable rectangular button.
 */
UCLASS( ClassGroup = UXTools, meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtPressableButtonComponent : public USceneComponent, public IUxtPokeTarget, public IUxtFarTarget
{
	GENERATED_BODY()

public:

	UUxtPressableButtonComponent();

	/** Get the distance from the visuals front face to the collider front face. */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	float GetFrontFaceCollisionFraction() const;

	/** Set the distance from the visuals front face to the collider front face. */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	void SetFrontFaceCollisionFraction(float Distance);

	/** Get scene component used for the moving visuals */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	USceneComponent* GetVisuals() const;

	/** Set scene component to be used for the moving visuals */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	void SetVisuals(USceneComponent* Visuals);

	/** Set collision profile used by the button collider */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	void SetCollisionProfile(FName Profile);

	/** Switch between world and local space for button distances */
	UFUNCTION(BlueprintSetter, Category = "Pressable Button")
	void SetUseAbsolutePushDistance(bool bAbsolute);

	/** Set if the button is enabled */
	UFUNCTION(BlueprintCallable, Category = "Pressable Button")
	void SetEnabled(bool Enabled);

	/** Get the current state of the button */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	EUxtButtonState GetState() const;

	/** Gets the maximum distance the button can be pushed scaled by the transform's 'x' scale.*/
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	float GetScaleAdjustedMaxPushDistance() const;

	/** Gets the button behavior when pushed */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	EUxtPushBehavior GetPushBehavior() const;

	/** Sets the button behavior when pushed */
	UFUNCTION(BlueprintSetter, Category = "Pressable Button")
	void SetPushBehavior(EUxtPushBehavior Behavior);

	/** Gets the maximum distance the button can be pushed */
	UFUNCTION(BlueprintPure, Category = "Pressable Button")
	float GetMaxPushDistance() const;

	/** Sets the maximum distance the button can be pushed, does nothing when the push behavior is set to compress because the maximum distance is auto calculated */
	UFUNCTION(BlueprintSetter, Category = "Pressable Button")
	void SetMaxPushDistance(float Distance);

	/** Filter function used by the button when calculating the hierarchy bounds of the visuals */
	static bool VisualBoundsFilter(const USceneComponent* Component);

	/** Fraction of the maximum travel distance at which the button will raise the pressed event. */
	UPROPERTY(EditAnywhere, meta = (DisplayAfter = "bUseAbsolutePushDistance"), Category = "Pressable Button")
    float PressedFraction = 0.5f;

	/** Fraction of the maximum travel distance at which a pressed button will raise the released event. */
	UPROPERTY(EditAnywhere, meta = (DisplayAfter = "bUseAbsolutePushDistance"), Category = "Pressable Button")
    float ReleasedFraction = 0.2f;

	/** Button movement speed while recovering */
	UPROPERTY(EditAnywhere, meta = (DisplayAfter = "bUseAbsolutePushDistance"), Category = "Pressable Button")
    float RecoverySpeed = 50;

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

	/** Event raised when a pointer starts poking the Pressable Button. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonBeginPokeDelegate OnBeginPoke;

	/** Event raised while a pointer is poking the Pressable Button. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonUpdatePokeDelegate OnUpdatePoke;

	/** Event raised when a pointer ends poking the Pressable Button. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonEndPokeDelegate OnEndPoke;

	/** Event raised when the button reaches the pressed distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonPressedDelegate OnButtonPressed;

	/** Event raised when the a pressed button reaches the released distance. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonReleasedDelegate OnButtonReleased;

	/** Event raised when the button is enabled. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonEnabledDelegate OnButtonEnabled;

	/** Event raised when the button is disabled. */
	UPROPERTY(BlueprintAssignable, Category = "Pressable Button")
	FUxtButtonDisabledDelegate OnButtonDisabled;

protected:

	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//
	// UObject interface

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* Property) const override;
#endif

	//
	// IUxtPokeTarget interface

	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual EUxtPokeBehaviour GetPokeBehaviour_Implementation() const override;

	//
	// IUxtFarTarget interface

	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:

	/** Get the current contact state of the button */
	bool IsContacted() const;

	/** Get the current focus state of the button */
	bool IsFocused() const;

	/** Generic handler for enter focus events. */
	void OnEnterFocus(UUxtPointerComponent* Pointer);

	/** Generic handler for exit focus events. */
	void OnExitFocus(UUxtPointerComponent* Pointer);

	/** The local space distance at which the button will fire a pressed event */
	float GetPressedDistance() const;

	/** The local space distance at which the button will fire a released event */
	float GetReleasedDistance() const;

	/** Use the given scene component(s) to adjust the box component extents. */
	void ConfigureBoxComponent(USceneComponent* Parent);

	/** Returns the distance a given pointer is pushing the button to in local space. */
	float CalculatePushDistance(const UUxtNearPointerComponent* pointer) const;

	/** Get the current pushed position of the button */
	FVector GetCurrentButtonLocation() const;

	/** Position of the button front face while not being poked by any pointer */
	FVector GetRestPosition() const;

	/** Updates the button distance values so that absolute distances do not change. */
	void UpdateButtonDistancesScale();

	/** Updates the max push distance as the 'x' bounds of the box component when the button is compressible */
	void UpdateMaxPushDistance();

	/** Button behavior when pushed */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetPushBehavior", BlueprintSetter = "SetPushBehavior", Category = "Pressable Button")
	EUxtPushBehavior PushBehavior = EUxtPushBehavior::Translate;

	/** The maximum distance the button can be pushed in local space (auto-calculated when the push behavior is compress)*/
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetMaxPushDistance", BlueprintSetter = "SetMaxPushDistance", Category = "Pressable Button")
	float MaxPushDistance = 10;

	/** Distance from the visuals front face to the collider front face expressed as a fraction of the maximum push distance. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetFrontFaceCollisionFraction", BlueprintSetter = "SetFrontFaceCollisionFraction", meta = (UIMin = "0.0"), Category = "Pressable Button")
	float FrontFaceCollisionFraction = 0.05f;
    
	/** Visual representation of the button face. This component's transform will be updated as the button is pressed/released. */
	UPROPERTY(EditAnywhere, DisplayName = "Visuals", meta = (UseComponentPicker, AllowedClasses = "USceneComponent"), Category = "Pressable Button")
	FComponentReference VisualsReference;

	/** Collision profile used by the button collider */
	UPROPERTY(EditAnywhere, Category = "Pressable Button")
	FName CollisionProfile = TEXT("UI");
	
	/** Switch between world and local space for button distances */
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetUseAbsolutePushDistance", meta = (DisplayAfter = "MaxPushDistance"), Category = "Pressable Button")
	bool bUseAbsolutePushDistance = false;

	/** Set the pressed state of the button and trigger corresponding events */
	void SetPressed(bool bPressedState, UUxtPointerComponent* Pointer, bool bRaiseEvents = true);

	/** Number of pointers currently focusing the button. */
	int NumPointersFocusing = 0;

	/** List of currently poking pointers. */
	TSet<UUxtNearPointerComponent*> PokePointers;
	
	/** Far pointer currently pressing the button if any */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	/** Collision volume used for determining poke events */
	UBoxComponent* BoxComponent;

	/** Visuals offset in this component's space */
	FVector VisualsOffsetLocal;

	/** Visuals scale in this component's space */
	FVector VisualsScaleLocal;

	/** True if the button is currently pressed */
	bool bIsPressed = false;

	/** True if the button is currently disabled */
	bool bIsDisabled = false;

	/** Local position of the button front face while not being poked by any pointer */
	FVector RestPositionLocal;

	/** The current pushed distance of from poking pointers */
	float CurrentPushDistance;
};
