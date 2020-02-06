// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "UxtTouchPointer.generated.h"


/**
 * Turns an actor into a touch pointer.
 * Touch pointers keep track of all overlapping touch targets and raise hover events on the closest one.
 * Touch targets use the transform of touch pointers hovering them to drive their interactions.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTouchPointer : public USceneComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUxtTouchPointer();

	UFUNCTION(BlueprintSetter)
	void SetTouchRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetTouchRadius() const;

	/** 
	 * Returns currently hovered touch target or null if there is none. 
	 * The closest point on the target surface is stored in OutClosestPointOnTarget.
	 */
	UFUNCTION(BlueprintPure, Category = "Touch Pointer")
	UObject* GetHoveredTarget(FVector& OutClosestPointOnTarget) const;

	/**
	 * Set a hovered touch target explicitly which will receive grasp events.
	 * If bEnableHoverLock is true, then the new hover target will be locked until released by calling SetHoverLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Pointer")
	bool SetHoveredTarget(UActorComponent* NewHoveredTarget, bool bEnableHoverLock);

	/** Returns whether the pointer is locked on the currently hovered target. */
	UFUNCTION(BlueprintGetter)
	bool GetHoverLocked() const;

	/** Sets whether the pointer is locked on the currently hovered target. */
	UFUNCTION(BlueprintSetter)
	void SetHoverLocked(bool Value);

	UFUNCTION(BlueprintGetter)
	bool GetGrasped() const;

	UFUNCTION(BlueprintSetter)
	void SetGrasped(bool Enable);

	/**
	 * Get the default target object.
	 * This object receives hover and grasp events when no other target is hovered.
	 */
	UFUNCTION(BlueprintPure, Category = "Touch Pointer")
	UObject* GetDefaultTarget() const;

	/**
	 * Set the default target object.
	 * This object receives hover and grasp events when no other target is hovered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Touch Pointer")
	void SetDefaultTarget(UObject* NewDefaultTarget);

protected:

	// 
	// UActorComponent interface

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** Change the hovered target. Does not check hover lock state. */
	void ChangeHoveredTarget(UActorComponent* NewHoveredTarget, const FVector& NewClosestPointOnTarget);

protected:

	/** Weak reference to the currently hovered target. */
	TWeakObjectPtr<UObject> HoveredTargetWeak;

	/** Closest point on the surface of the hovered target. */
	FVector ClosestPointOnHoveredTarget = FVector::ZeroVector;

private:

	/**
	 * Whether the pointer is locked on its current hovered target.
	 * When locked, pointers won't change their hovered target even if they stop overlapping it.
	 */
	UPROPERTY(BlueprintGetter = "GetHoverLocked", BlueprintSetter = "SetHoverLocked", Category = "Touch Pointer")
	bool bHoverLocked = false;

	UPROPERTY(BlueprintGetter = "GetGrasped", BlueprintSetter = "SetGrasped", Category = "Touch Pointer")
	bool bIsGrasped = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Touch Pointer", meta = (AllowPrivateAccess = "true"))
	USphereComponent* TouchSphere = nullptr;

	UPROPERTY(EditAnywhere, BlueprintSetter = "SetTouchRadius", BlueprintGetter = "GetTouchRadius", Category = "Touch Pointer")
	float TouchRadius = 10.0f;

	/**
	 * Optional weak reference to a default target object
	 * that receives hover and grasp events if no other target is hovered.
	 */
	TWeakObjectPtr<UObject> DefaultTargetWeak;
};
