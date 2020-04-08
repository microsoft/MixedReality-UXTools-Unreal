// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UxtNearPointerComponent.generated.h"

struct FUxtGrabPointerFocus;
struct FUxtPokePointerFocus;

/**
 * Adds poke and grab interactions to an actor.
 * It keeps track of all overlapping poke targets and raises focus events on the closest one.
 * Targets use the transform of pointers focusing them to drive their interactions.
 */
UCLASS(ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtNearPointerComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UUxtNearPointerComponent();
	virtual ~UUxtNearPointerComponent();

	// 
	// UActorComponent interface

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void SetActive(bool bNewActive, bool bReset = false) override;

	/** Returns currently focused grab target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetFocusedGrabTarget(FVector& OutClosestPointOnTarget) const;

	/** Returns currently focused poke target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	UObject* GetFocusedPokeTarget(FVector& OutClosestPointOnTarget) const;

	/**
	 * Set a focused grab target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	bool SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/**
	 * Set a focused poke target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Hand Pointer")
	bool SetFocusedPokeTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/** Returns whether the pointer is locked on the currently focused target. */
	UFUNCTION(BlueprintGetter)
	bool GetFocusLocked() const;

	/** Sets whether the pointer is locked on the currently focused target. */
	UFUNCTION(BlueprintSetter)
	void SetFocusLocked(bool Value);

	UFUNCTION(BlueprintGetter)
	bool IsGrabbing() const;
    
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	bool GetIsPoking() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetGrabPointerTransform() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	FTransform GetPokePointerTransform() const;
	UFUNCTION(BlueprintPure, Category = "Hand Pointer")
	float GetPokePointerRadius() const;

	/** The hand that this component represents.
	 *  Determines the position of touch and grab pointers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Pointer")
	EControllerHand Hand = EControllerHand::Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Pointer")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Pointer")
	float ProximityRadius = 11.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Pointer")
	float PokeRadius = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Pointer")
	float GrabRadius = 3.5f;

	/**
	 * The depth beyond the front face at which a front face pokable no longer recieves poke events.
	 * While poking a front face pokable, if the near pointer moves beyond this depth, the pokable
	 * will receive a poke end event.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Pointer")
	float PokeDepth = 20.0f;

	/**
	 * Whether the pointer is locked on its current focused target.
	 * When locked, pointers won't change their focused target even if they stop overlapping it.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "Hand Pointer")
	bool bFocusLocked = false;

protected:

	/** Focus of the grab pointer */
	FUxtGrabPointerFocus* GrabFocus;

	/** Focus of the poke pointer */
	FUxtPokePointerFocus* PokeFocus;

private:

	FTransform GrabPointerTransform;

	FTransform PokePointerTransform;

	bool bIsPoking = false;

	FVector PreviousPokePointerLocation;

	bool bWasBehindFrontFace = false;
};
