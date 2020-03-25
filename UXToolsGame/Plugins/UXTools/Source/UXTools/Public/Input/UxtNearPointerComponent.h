// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Input/UxtPointerTypes.h"
#include "UxtNearPointerComponent.generated.h"

/**
 * Adds touch and grab interactions to an actor.
 * It keeps track of all overlapping touch targets and raises focus events on the closest one.
 * Targets use the transform of pointers focusing them to drive their interactions.
 */
UCLASS(ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtNearPointerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUxtNearPointerComponent();

	UFUNCTION(BlueprintGetter)
	EControllerHand GetHand() const;
	UFUNCTION(BlueprintSetter)
	void SetHand(EControllerHand NewHand);

	UFUNCTION(BlueprintGetter)
	ECollisionChannel GetTraceChannel() const;
	UFUNCTION(BlueprintSetter)
	void SetTraceChannel(ECollisionChannel NewTraceChannel);

	UFUNCTION(BlueprintGetter)
	float GetProximityRadius() const;
	UFUNCTION(BlueprintSetter)
	void SetProximityRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetTouchRadius() const;
	UFUNCTION(BlueprintSetter)
	void SetTouchRadius(float radius);

	UFUNCTION(BlueprintGetter)
	float GetGrabRadius() const;
	UFUNCTION(BlueprintSetter)
	void SetGrabRadius(float radius);

	/** Returns currently focused grab target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetFocusedGrabTarget(FVector& OutClosestPointOnTarget) const;

	/** Returns currently focused touch target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetFocusedTouchTarget(FVector& OutClosestPointOnTarget) const;

	/**
	 * Set a focused grab target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	bool SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/**
	 * Set a focused touch target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	bool SetFocusedTouchTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/** Returns whether the pointer is locked on the currently focused target. */
	UFUNCTION(BlueprintGetter)
	bool GetFocusLocked() const;

	/** Sets whether the pointer is locked on the currently focused target. */
	UFUNCTION(BlueprintSetter)
	void SetFocusLocked(bool Value);

	UFUNCTION(BlueprintGetter)
	bool IsGrabbing() const;
	UFUNCTION(BlueprintSetter)
	void SetGrabbing(bool Enable);

	UFUNCTION(BlueprintGetter)
	FTransform GetIndexTipTransform() const;
	UFUNCTION(BlueprintSetter)
	void SetIndexTipTransform(const FTransform& NewTransform);

	UFUNCTION(BlueprintGetter)
	FTransform GetThumbTipTransform() const;
	UFUNCTION(BlueprintSetter)
	void SetThumbTipTransform(const FTransform& NewTransform);

	/**
	 * Get the default target object.
	 * This object receives focus and grasp events when no other target is focused.
	 */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetDefaultTarget() const;

	/**
	 * Set the default target object.
	 * This object receives focus and grasp events when no other target is focused.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	void SetDefaultTarget(UObject* NewDefaultTarget);

	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetGrabPointerTransform() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetTouchPointerTransform() const;

protected:

	// 
	// UActorComponent interface

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** Change the focused target. Does not check focus lock state. */
	void ChangeFocusedGrabTarget(UActorComponent* NewFocusedTarget, const FVector& NewClosestPointOnTarget);
	/** Change the focused target. Does not check focus lock state. */
	void ChangeFocusedTouchTarget(UActorComponent* NewFocusedTarget, const FVector& NewClosestPointOnTarget);

	/** Change the focused target to the closest target among the given overlaps. Does not check focus lock state. */
	void FocusClosestGrabTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point);
	/** Change the focused target to the closest target among the given overlaps. Does not check focus lock state. */
	void FocusClosestTouchTarget(const TArray<FOverlapResult>& Overlaps, const FVector& Point);

protected:

	/** Weak reference to the currently focused grab target. */
	TWeakObjectPtr<UObject> FocusedGrabTargetWeak;
	/** Weak reference to the currently focused touch target. */
	TWeakObjectPtr<UObject> FocusedTouchTargetWeak;

	/** Closest point on the surface of the focused grab target. */
	FVector ClosestGrabTargetPoint = FVector::ZeroVector;
	/** Closest point on the surface of the focused touch target. */
	FVector ClosestTouchTargetPoint = FVector::ZeroVector;

private:

	/** The hand that this component represents.
	 *  Determines the position of touch and grab pointers.
	 */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetHand", BlueprintSetter = "SetHand", Category = "Hand Pointer")
	EControllerHand Hand = EControllerHand::Right;

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetTraceChannel", BlueprintSetter = "SetTraceChannel", Category = "Hand Pointer")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_WorldDynamic;

	UPROPERTY(EditAnywhere, BlueprintSetter = "SetProximityRadius", BlueprintGetter = "GetProximityRadius", Category = "Hand Pointer")
	float ProximityRadius = 11.0f;
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetTouchRadius", BlueprintGetter = "GetTouchRadius", Category = "Hand Pointer")
	float TouchRadius = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintSetter = "SetGrabRadius", BlueprintGetter = "GetGrabRadius", Category = "Hand Pointer")
	float GrabRadius = 3.5f;

	/**
	 * Whether the pointer is locked on its current focused target.
	 * When locked, pointers won't change their focused target even if they stop overlapping it.
	 */
	UPROPERTY(BlueprintGetter = "GetFocusLocked", BlueprintSetter = "SetFocusLocked", Category = "Hand Pointer")
	bool bFocusLocked = false;

	UPROPERTY(BlueprintGetter = "IsGrabbing", BlueprintSetter = "SetGrabbing", Category = "Hand Pointer")
	bool bIsGrabbing = false;

	UPROPERTY(BlueprintGetter = "GetIndexTipTransform", BlueprintSetter = "SetIndexTipTransform", Category = "Hand Pointer")
	FTransform IndexTipTransform;
	UPROPERTY(BlueprintGetter = "GetThumbTipTransform", BlueprintSetter = "SetThumbTipTransform", Category = "Hand Pointer")
	FTransform ThumbTipTransform;

	/**
	 * Optional weak reference to a default target object
	 * that receives focus and grab events if no other target is focused.
	 */
	TWeakObjectPtr<UObject> DefaultTargetWeak;
};
