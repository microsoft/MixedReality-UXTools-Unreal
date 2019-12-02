// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "TouchPointer.generated.h"


class UTouchPointer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTouchPointerBeginPinchDelegate, UTouchPointer*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTouchPointerEndPinchDelegate, UTouchPointer*, Pointer);

/**
 * Turns an actor into a touch pointer.
 * Touch pointers confer focus to interactables (e.g. buttons) when they overlap with them. 
 * Interactables use the transform of pointers focusing them to drive their interactions.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXEDREALITYUTILS_API UTouchPointer : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UTouchPointer();

	UFUNCTION(BlueprintSetter)
	void SetTouchRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetTouchRadius() const;

	/** 
	 * Returns the target that is closest to the pointer position or null if there is none. 
	 * The closest point on the target surface is stored in OutPointOnTargetSurface.
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Pointer")
	UActorComponent* GetClosestPointOnTargets(FVector& OutPointOnTargetSurface) const;

	UFUNCTION(BlueprintGetter)
	bool GetGrasped() const;

	UFUNCTION(BlueprintSetter)
	void SetGrasped(bool Enable);

	/** Event raised when the pointer pinch starts. */
	UPROPERTY(BlueprintAssignable, Category = "Touch Pointer")
	FTouchPointerBeginPinchDelegate OnBeginPinch;

	/** Event raised when the pointer pinch ends. */
	UPROPERTY(BlueprintAssignable, Category = "Touch Pointer")
	FTouchPointerEndPinchDelegate OnEndPinch;

protected:

	/// Start touching the actor.
	/// Returns false if the actor has no valid touch target.
	bool TryStartTouching(AActor* actor);

	/// Stop touching the actor.
	/// Returns false if the actor was not touched.
	bool TryStopTouching(AActor* actor);

	/// Stop touching all current targets.
	void StopAllTouching();

	UFUNCTION()
	void OnPointerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void OnPointerEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	bool ImplementsTargetInterface(const UObject *obj) const;

protected:

	TSet<UActorComponent*> TouchedTargets;

private:

	UPROPERTY(BlueprintGetter = "GetGrasped", BlueprintSetter = "SetGrasped", Category = "Touch Pointer")
	bool bIsGrasped;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touch Pointer", meta = (AllowPrivateAccess = "true"))
	USphereComponent *TouchSphere;

	UPROPERTY(EditAnywhere, BlueprintSetter = "SetTouchRadius", BlueprintGetter = "GetTouchRadius", Category = "Touch Pointer")
	float TouchRadius;
};
