// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "Controls/UxtUIElementComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"

#include "UxtTouchableVolumeComponent.generated.h"

class UUxtTouchableVolumeComponent;
class UUxtFarPointerComponent;
class UBoxComponent;
class UShapeComponent;

//
// Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtVolumeBeginFocusDelegate, UUxtTouchableVolumeComponent*, Volume, UObject*, Pointer, bool, bWasAlreadyFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtVolumeUpdateFocusDelegate, UUxtTouchableVolumeComponent*, Volume, UObject*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtVolumeEndFocusDelegate, UUxtTouchableVolumeComponent*, Volume, UObject*, Pointer, bool, bIsStillFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtVolumeBeginPokeDelegate, UUxtTouchableVolumeComponent*, Volume, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtVolumeUpdatePokeDelegate, UUxtTouchableVolumeComponent*, Volume, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtVolumeEndPokeDelegate, UUxtTouchableVolumeComponent*, Volume, UUxtNearPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtVolumeEnabledDelegate, UUxtTouchableVolumeComponent*, Volume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtVolumeDisabledDelegate, UUxtTouchableVolumeComponent*, Volume);

/**
 * Component that turns the actor it is attached to into a touchable volume.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTouchableVolumeComponent
	: public UUxtUIElementComponent
	, public IUxtPokeTarget
	, public IUxtPokeHandler
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	/** Set if the touchable volume is enabled */
	UFUNCTION(BlueprintCallable, Category = "Uxt Touchable Volume")
	void SetEnabled(bool Enabled);

	/** List of primitives used as touchable targets.
	 * If the list is empty then all primitives of the actor are used.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Uxt Touchable Volume")
	TSet<UPrimitiveComponent*> TouchablePrimitives;

	//
	// Events

	/** Event raised when a pointer starts focusing the touchable volume. WasFocused indicates if the volume was already focused by another
	 * pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeBeginFocusDelegate OnBeginFocus;

	/** Event raised when a focusing pointer updates. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeUpdateFocusDelegate OnUpdateFocus;

	/** Event raised when a pointer ends focusing the touchable volume. IsFocused indicates if the volume is still focused by another
	 * pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeEndFocusDelegate OnEndFocus;

	/** Event raised when a pointer starts poking the touchable volume. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeBeginPokeDelegate OnBeginPoke;

	/** Event raised while a pointer is poking the touchable volume. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeUpdatePokeDelegate OnUpdatePoke;

	/** Event raised when a pointer ends poking the touchable volume. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeEndPokeDelegate OnEndPoke;

	/** Event raised when the volume is enabled. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeEnabledDelegate OnVolumeEnabled;

	/** Event raised when the volume is disabled. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Touchable Volume")
	FUxtVolumeDisabledDelegate OnVolumeDisabled;

protected:
	//
	// UActorComponent interface
	virtual void BeginPlay() override;

	//
	// IUxtPokeTarget interface
	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;
	virtual EUxtPokeBehaviour GetPokeBehaviour_Implementation() const override;
	virtual bool GetClosestPoint_Implementation(
		const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const override;

	//
	// IUxtPokeHandler interface
	virtual bool CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

	//
	// Touch input handlers
	UFUNCTION(Category = "Uxt Touchable Volume")
	void OnInputTouchBeginHandler(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);
	UFUNCTION(Category = "Uxt Touchable Volume")
	void OnInputTouchEndHandler(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);
	UFUNCTION(Category = "Uxt Touchable Volume")
	void OnInputTouchLeaveHandler(ETouchIndex::Type FingerIndex, UPrimitiveComponent* TouchedComponent);

private:
	/** Get the current contact state of the volume */
	bool IsContacted() const;

	/** Get the current focus state of the volume */
	bool IsFocused() const;

	/** Generic handler for enter focus events. */
	void OnEnterFocus(UObject* Pointer);

	/** Generic handler for exit focus events. */
	void OnExitFocus(UObject* Pointer);

	/** True if the volume is currently disabled */
	bool bIsDisabled = false;

	/** Number of pointers currently focusing the button. */
	int NumPointersFocusing = 0;

	/** List of currently poking pointers. */
	TSet<UUxtNearPointerComponent*> PokePointers;

	/** Far pointer currently pressing the button if any */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	/** Active input touches */
	TSet<ETouchIndex::Type> ActiveTouches;
};
