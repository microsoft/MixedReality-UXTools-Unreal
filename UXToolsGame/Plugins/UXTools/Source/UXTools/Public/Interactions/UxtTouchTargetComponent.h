// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Interactions/UxtTouchTarget.h"

#include "UxtTouchTargetComponent.generated.h"

class UUxtTouchTargetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBeginFocusDelegate, UUxtTouchTargetComponent*, Interactable, UUxtNearPointerComponent*, Pointer, FUxtPointerInteractionData, Data, bool, bWasFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUpdateFocusDelegate, UUxtTouchTargetComponent*, Interactable, UUxtNearPointerComponent*, Pointer, FUxtPointerInteractionData, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEndFocusDelegate, UUxtTouchTargetComponent*, Interactable, UUxtNearPointerComponent*, Pointer, bool, bIsFocused);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FBeginTouchDelegate, UUxtTouchTargetComponent*, Interactable, UUxtNearPointerComponent*, Pointer, FUxtPointerInteractionData, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUpdateTouchDelegate, UUxtTouchTargetComponent*, Interactable, UUxtNearPointerComponent*, Pointer, FUxtPointerInteractionData, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEndTouchDelegate, UUxtTouchTargetComponent*, Interactable, UUxtNearPointerComponent*, Pointer);

/**
 * Base class for pointer targets that keeps track of the currently touching pointers.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTouchTargetComponent : public USceneComponent, public IUxtTouchTarget
{
    GENERATED_BODY()

public:

	UUxtTouchTargetComponent();

protected:
	//
	// IUxtTouchTarget interface

	virtual void OnEnterTouchFocus_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) override;
	virtual void OnUpdateTouchFocus_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) override;
	virtual void OnExitTouchFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual bool GetClosestTouchPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutPointOnSurface) const override;

	virtual void OnBeginTouch_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) override;
	virtual void OnUpdateTouch_Implementation(UUxtNearPointerComponent* Pointer, const FUxtPointerInteractionData& Data) override;
	virtual void OnEndTouch_Implementation(UUxtNearPointerComponent* Pointer) override;

	/** Returns a list of the pointers that are currently touching this actor. */
	const TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData>& GetFocusedPointers() const;

	/** Returns a list of all currently grabbing pointers. */
	const TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData>& GetTouchPointers() const;

public:	

	/** Event raised when a pointer starts focusing the interactable. WasFocused indicates if the interactable was already focused by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FBeginFocusDelegate OnBeginFocus;

	/** Event raised when a focusing pointer updates. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FUpdateFocusDelegate OnUpdateFocus;

	/** Event raised when a pointer ends focusing the interactable. IsFocused indicates if the interactable is still focused by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FEndFocusDelegate OnEndFocus;

	/** Event raised when a pointer starts touching the interactable. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FBeginTouchDelegate OnBeginTouch;

	/** Event raised while a pointer is touching the interactable. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FUpdateTouchDelegate OnUpdateTouch;

	/** Event raised when a pointer ends touching the interactable. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FEndTouchDelegate OnEndTouch;

private:

	/** List of pointers that are currently touching the actor. */
	TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData> FocusedPointers;

	/** List of currently grabbing pointers. */
	TMap<UUxtNearPointerComponent*, FUxtPointerInteractionData> TouchPointers;
};
