// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Input/UxtPointerTypes.h"
#include "UxtNearPointerComponent.generated.h"

struct FUxtGrabPointerFocus;
struct FUxtTouchPointerFocus;

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

	UUxtNearPointerComponent();
	virtual ~UUxtNearPointerComponent();

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

	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetGrabPointerTransform() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetTouchPointerTransform() const;

	/**
	 * Get the default target object of the grab pointer.
	 * This object receives focus and grab events when no other target is focused.
	 */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetDefaultGrabTarget() const;
	/**
	 * Set the default target object of the grab pointer.
	 * This object receives focus and grab events when no other target is focused.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	void SetDefaultGrabTarget(UObject* NewDefaultTarget);

	/**
	 * Get the default target object of the touch pointer.
	 * This object receives focus and touch events when no other target is focused.
	 */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetDefaultTouchTarget() const;
	/**
	 * Set the default target object of the grab pointer.
	 * This object receives focus and touch events when no other target is focused.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	void SetDefaultTouchTarget(UObject* NewDefaultTarget);

protected:

	// 
	// UActorComponent interface

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	/** Focus of the grab pointer */
	FUxtGrabPointerFocus* GrabFocus;

	/** Focus of the touch pointer */
	FUxtTouchPointerFocus* TouchFocus;

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

};
