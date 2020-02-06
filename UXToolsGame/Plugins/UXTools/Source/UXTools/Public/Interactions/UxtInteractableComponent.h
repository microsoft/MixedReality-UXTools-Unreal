// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UxtTouchPointerTarget.h"

#include "UxtInteractableComponent.generated.h"

class UUxtInteractableComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHoverStartedDelegate, UUxtInteractableComponent*, Interactable, USceneComponent*, Pointer, bool, bWasHovered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHoverEndedDelegate, UUxtInteractableComponent*, Interactable, USceneComponent*, Pointer, bool, bIsHovered);

/**
 * Base class for pointer targets that keeps track of the currently touching pointers.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtInteractableComponent : public USceneComponent, public IUxtTouchPointerTarget
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

	/** Returns a list of the pointers that are currently touching this actor. */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	TArray<UUxtTouchPointer*> GetActivePointers() const;

private:

	/** List of pointers that are currently touching the actor. */
	TSet<TWeakObjectPtr<UUxtTouchPointer>> ActivePointers;

	/** Event raised when a pointer starts hovering the interactable. WasHovered indicates if the interactable was already hovered by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FHoverStartedDelegate OnHoverStarted;

	/** Event raised when a pointer ends hovering the interactable. IsHovered indicates if the interactable is still hovered by another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Interactable")
	FHoverEndedDelegate OnHoverEnded;
};
