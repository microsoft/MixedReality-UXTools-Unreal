// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EngineDefines.h"
#include "InputCoreTypes.h"
#include "UxtPointerComponent.h"

#include "Components/ActorComponent.h"

#include "UxtNearPointerComponent.generated.h"

struct FUxtPointerFocus;
struct FUxtGrabPointerFocus;
struct FUxtPokePointerFocus;
class UMaterialParameterCollection;

/**
 * Adds poke and grab interactions to an actor.
 * It keeps track of all overlapping poke targets and raises focus events on the closest one.
 * Targets use the transform of pointers focusing them to drive their interactions.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtNearPointerComponent : public UUxtPointerComponent
{
	GENERATED_BODY()

public:
	UUxtNearPointerComponent();
	virtual ~UUxtNearPointerComponent();

	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void SetActive(bool bNewActive, bool bReset = false) override;

	//
	// UUxtPointerComponent interface

	virtual UObject* GetFocusTarget() const override;
	virtual FTransform GetCursorTransform() const override;

	/** Update poke distances and detect if poking the target. */
	void UpdatePokeInteraction();

	/** Returns currently focused grab target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	UObject* GetFocusedGrabTarget(FVector& OutClosestPointOnTarget, FVector& Normal) const;

	/** Returns currently focused poke target or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	UObject* GetFocusedPokeTarget(FVector& OutClosestPointOnTarget, FVector& Normal) const;

	/** Returns currently focused grab primitive or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	UPrimitiveComponent* GetFocusedGrabPrimitive(FVector& OutClosestPointOnTarget, FVector& Normal) const;

	/** Returns currently focused poke primitive or null if there is none. */
	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	UPrimitiveComponent* GetFocusedPokePrimitive(FVector& OutClosestPointOnTarget, FVector& Normal) const;

	/**
	 * Set a focused grab target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Near Pointer")
	bool SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	/**
	 * Set a focused poke target explicitly which will receive grasp events.
	 * If bEnableFocusLock is true, then the new focus target will be locked until released by calling SetFocusLocked.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Near Pointer")
	bool SetFocusedPokeTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock);

	UFUNCTION(BlueprintGetter, Category = "Uxt Near Pointer")
	bool IsGrabbing() const;

	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	bool GetIsPoking() const;

	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	FTransform GetGrabPointerTransform() const;

	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	FTransform GetPokePointerTransform() const;

	UFUNCTION(BlueprintPure, Category = "Uxt Near Pointer")
	float GetPokePointerRadius() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Near Pointer")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Near Pointer")
	float ProximityRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Near Pointer")
	float PokeRadius = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Near Pointer")
	float GrabRadius = 3.5f;

	/**
	 * The depth beyond the front face at which a front face pokable no longer recieves poke events.
	 * While poking a front face pokable, if the near pointer moves beyond this depth, the pokable
	 * will receive a poke end event.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Near Pointer")
	float PokeDepth = 30.0f;

	/**
	 * The distance the fingertip must be from a pokeable in order to fire a poke end event. This is
	 * used in order to distinguish the queries for poke begin and poke end so you cannot easily
	 * cause end touch to fire one frame and begin touch to fire on the next frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Near Pointer")
	float DebounceDepth = 0.5f;

protected:
	/** Focus of the grab pointer */
	FUxtGrabPointerFocus* GrabFocus;

	/** Focus of the poke pointer */
	FUxtPokePointerFocus* PokeFocus;

private:
	void UpdateParameterCollection(FVector IndexTipPosition);

#if ENABLE_VISUAL_LOG
	void VLogPointer(
		const FName& LogCategoryName, const FColor& LogColor, const FString& Label, const FVector& PointerLocation, float PointerRadius,
		const FUxtPointerFocus* Focus) const;
#endif // ENABLE_VISUAL_LOG

	/** Parameter collection used to store the finger tip position */
	UPROPERTY(Transient)
	UMaterialParameterCollection* ParameterCollection;

	FTransform GrabPointerTransform;

	FTransform PokePointerTransform;

	FVector PreviousPokePointerLocation;

	bool bWasBehindFrontFace = false;

	bool bHandWasGrabbing = false;
};
