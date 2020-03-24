// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UxtTouchPointerTarget.h"
#include "Interactions/UxtFarTarget.h"
#include "UxtInteractableComponent.generated.h"

class UUxtInteractableComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHoverStartedDelegate, UUxtInteractableComponent*, Interactable, UObject*, Pointer, bool, bWasHovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHoverEndedDelegate, UUxtInteractableComponent*, Interactable, UObject*, Pointer, bool, bIsHovered);

/**
 * Base class for pointer targets that keeps track of the currently touching pointers.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtInteractableComponent : public USceneComponent, public IUxtTouchPointerTarget, public IUxtFarTarget
{
    GENERATED_BODY()

public:

	UUxtInteractableComponent();

protected:
	//
	// ITouchPointerTarget interface

	virtual void HoverStarted_Implementation(UUxtTouchPointer* Pointer) override;
	virtual void HoverEnded_Implementation(UUxtTouchPointer* Pointer) override;

	virtual bool GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface) override;

	//
	// IUxtFarTarget interface

	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent) override;

	/** Returns a list of the pointers that are currently touching this actor. */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	TArray<UUxtTouchPointer*> GetActiveTouchPointers() const;

private:

	/** List of touch pointers that are currently touching the actor. */
	TSet<TWeakObjectPtr<UUxtTouchPointer>> ActiveTouchPointers;

	/** List of far pointers that are currently focusing the actor. */
	TSet<TWeakObjectPtr<UUxtFarPointerComponent>> ActiveFarPointers;

	/** Event raised when a pointer starts hovering the interactable. WasHovered indicates if the interactable was already hovered by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FHoverStartedDelegate OnHoverStarted;

	/** Event raised when a pointer ends hovering the interactable. IsHovered indicates if the interactable is still hovered by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FHoverEndedDelegate OnHoverEnded;

	int NumPointersFocusing = 0;
};
