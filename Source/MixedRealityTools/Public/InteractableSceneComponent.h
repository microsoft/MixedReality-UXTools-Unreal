// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TouchPointerTarget.h"

#include "InteractableSceneComponent.generated.h"


UCLASS( ClassGroup = (Custom), meta = (BlueprintSpawnableComponent) )
class MIXEDREALITYTOOLS_API UInteractableSceneComponent : public USceneComponent, public ITouchPointerTarget
{
    GENERATED_BODY()

public:

    using PointerSet = TSet<TWeakObjectPtr<USceneComponent>>;

	UInteractableSceneComponent();

protected:

	//
	// ITouchPointerTarget interface

    virtual void TouchStarted_Implementation(USceneComponent* pointer) override;
    virtual void TouchEnded_Implementation(USceneComponent* pointer) override;
	virtual bool GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface) override;

    const PointerSet& GetActivePointers() const { return ActivePointers; }

private:

	TSet<TWeakObjectPtr<USceneComponent>> ActivePointers;

};
