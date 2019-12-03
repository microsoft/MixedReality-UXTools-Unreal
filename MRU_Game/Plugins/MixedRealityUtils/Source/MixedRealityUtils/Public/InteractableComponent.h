// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TouchPointerTarget.h"

#include "InteractableComponent.generated.h"


/**
 * Base class for pointer targets that keeps track of the currently touching pointers.
 */
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXEDREALITYUTILS_API UInteractableComponent : public USceneComponent, public ITouchPointerTarget
{
    GENERATED_BODY()

public:

	UInteractableComponent();

protected:
	//
	// ITouchPointerTarget interface

	virtual void HoverStarted_Implementation(UTouchPointer* Pointer) override;
	virtual void HoverEnded_Implementation(UTouchPointer* Pointer) override;

	virtual bool GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface) override;

	/** Returns a list of the pointers that are currently touching this actor. */
	UFUNCTION(BlueprintCallable, Category="Interactable")
	TArray<UTouchPointer*> GetActivePointers() const;

private:

	/** List of pointers that are currently touching the actor. */
	TSet<TWeakObjectPtr<UTouchPointer>> ActivePointers;

};
