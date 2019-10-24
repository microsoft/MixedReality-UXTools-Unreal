// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "TouchPointer.generated.h"


/**
 * Turns an actor into a touch pointer.
 * Touch pointers confer focus to interactables (e.g. buttons) when they overlap with them. 
 * Interactables use the transform of pointers focusing them to drive their interactions.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXEDREALITYTOOLS_API UTouchPointer : public USceneComponent
{
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UTouchPointer();

	/// Returns all active pointers.
	UFUNCTION(BlueprintCallable)
	static const TArray<UTouchPointer*>& GetAllPointers();

protected:

	/// Start touching the component.
	/// Returns false if the component is not a valid touch target.
	bool TryStartTouching(UActorComponent *comp);

	/// Stop touching the component.
	/// Returns false if the component was not touched.
	bool StopTouching(UActorComponent *comp);

	/// Stop touching all current components.
	void StopAllTouching();

	UFUNCTION()
	void OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	bool ImplementsTargetInterface(const UActorComponent *comp) const;

protected:

	TSet<TWeakObjectPtr<UActorComponent>> TouchedTargets;

private:

	/** List with all active pointers. */
	static TArray<UTouchPointer*> Pointers;
};
