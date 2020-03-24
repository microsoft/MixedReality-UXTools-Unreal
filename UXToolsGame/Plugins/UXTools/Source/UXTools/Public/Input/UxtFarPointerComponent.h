// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UxtFarPointerComponent.generated.h"

class UUxtFarPointerComponent;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtFarPointerEnabledDelegate, UUxtFarPointerComponent*, FarPointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtFarPointerDisabledDelegate, UUxtFarPointerComponent*, FarPointer);


/**
 * Component that casts a ray for the given hand-tracked hand and raises far interaction events on the far targets hit.
 * A far target is an actor or component implementing the UUxtFarTarget interface.
 */
UCLASS(ClassGroup = UXTools, meta=(BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFarPointerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UUxtFarPointerComponent();

	/** Origin of the pointer ray as reported by the hand tracker. See GetRayStart() for actual start of the ray used for querying the scene. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	FVector GetPointerOrigin() const;

	/** Orientation of the pointer ray. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	FQuat GetPointerOrientation() const;

	/** Start of the ray used for querying the scene. This is the pointer origin shifted by the ray start offset in the pointer forward direction. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	FVector GetRayStart() const;

	/** Primitive the pointer is currently hitting or null if none. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	UPrimitiveComponent* GetHitPrimitive() const;

	/** Current hit point location or ray end if there's no hit. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	FVector GetHitPoint() const;

	/** Current hit point normal or negative ray direction if there's no hit. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	FVector GetHitNormal() const;

	/** Whether the pointer is currently pressed. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	bool IsPressed() const;

	/** Whether the pointer is currently enabled. Hit information is only valid while the pointer is enabled. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	bool IsEnabled() const;

	/** Whether the pointer is currently locked. */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	bool GetFocusLocked() const;

	/** 
	  * Set the pointer's locked state. 
	  * Locked pointers don't update their hit, remaining focused on the primitive they were hitting until unlocked.
	  */
	UFUNCTION(BlueprintCallable, Category = "Far Pointer")
	void SetFocusLocked(bool bNewFocusLocked);

	// 
	// UActorComponent interface

	virtual void SetActive(bool bNewActive, bool bReset = false) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** Called every tick to update the pointer pose with the latest information from the hand tracker. */
	void OnPointerPoseUpdated(const FQuat& NewOrientation, const FVector& NewOrigin);

	/** Called every tick to update the pressed state with the latest information from the hand tracker. */
	void SetPressed(bool bNewPressed);

	/** Used to enable/disable the pointer on tracking gain/loss or component activation/deactivation. */
	void SetEnabled(bool bNewEnabled);

	/** Current far target if any. This will be a UObject implementing the UUxtFarTarget interface. */
	UObject* GetFarTarget() const;

public:

	/** Hand-tracked hand the pointer will use for targeting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Pointer")
	EControllerHand Hand;

	/** Trace channel to be used in the pointer's line trace query. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Pointer")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	/** Start of the pointer ray expressed as an offset from the hand ray origin in the ray direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Pointer")
	float RayStartOffset = 5;

	/** Pointer ray length from ray start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Pointer")
	float RayLength = 500;

	UPROPERTY(BlueprintAssignable, Category = "Far Pointer")
	FUxtFarPointerEnabledDelegate OnFarPointerEnabled;

	UPROPERTY(BlueprintAssignable, Category = "Far Pointer")
	FUxtFarPointerDisabledDelegate OnFarPointerDisabled;

private:

	/** Pointer origin as reported by the hand tracker. */
	FVector PointerOrigin;

	/** Pointer orientation. */
	FQuat PointerOrientation;

	TWeakObjectPtr<UPrimitiveComponent> HitPrimitiveWeak;
	FVector HitPoint = FVector::ZeroVector;
	FVector HitNormal = FVector::BackwardVector;

	// Hit information in the primitive space. Used to keep the hit relative to the primitive while the pointer is locked.
	FVector HitPointLocal = FVector::ZeroVector;
	FVector HitNormalLocal = FVector::BackwardVector;

	/** Far target that owns the hit primitive, if any. */
	TWeakObjectPtr<UObject> FarTargetWeak;

	bool bPressed = false;

	bool bFocusLocked = false;

	bool bEnabled = false;
};
