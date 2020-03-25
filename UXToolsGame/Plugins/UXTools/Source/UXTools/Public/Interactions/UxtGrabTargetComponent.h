// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/SceneComponent.h"
#include "Interactions/UxtGrabTarget.h"

#include "UxtGrabTargetComponent.generated.h"

/**
 * Utility struct that stores transient data for a pointer which is interacting with a grabbable component.
 */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtGrabPointerData
{
	GENERATED_BODY()

	/** The pointer that is interacting with the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	int32 PointerId;

	/** Last updated data of the pointer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	FUxtPointerInteractionData PointerData;

	/** The time at which interaction started, in seconds since application start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	float StartTime;

	/**
	 * Transform of the pointer when it started interacting, in the local space of the target component.
	 * This allows computing pointer offset in relation to the current actor transform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	FTransform LocalGrabPoint;
};

/**
 * Utility functions for FGrabPointerData.
 */
UCLASS()
class UXTOOLS_API UUxtGrabPointerDataFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Compute the grab point in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetGrabLocation(const FTransform &Transform, const FUxtGrabPointerData& GrabData);

	/** Compute the grab rotation in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetGrabRotation(const FTransform &Transform, const FUxtGrabPointerData& GrabData);

	/** Compute the grab transform in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FTransform GetGrabTransform(const FTransform &Transform, const FUxtGrabPointerData& GrabData);

	/** Compute the pointer target in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetTargetLocation(const FUxtGrabPointerData& GrabData);

	/** Compute the target rotation in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetTargetRotation(const FUxtGrabPointerData& GrabData);

	/** Compute the pointer target transform in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FTransform GetTargetTransform(const FUxtGrabPointerData& GrabData);

	/** Compute the world space offset between pointer grab point and target. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetLocationOffset(const FTransform &Transform, const FUxtGrabPointerData& GrabData);

	/** Compute the world space rotation between pointer grab point and target. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetRotationOffset(const FTransform &Transform, const FUxtGrabPointerData& GrabData);
};


/** Delegate for handling a BeginGrab event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtBeginGrabDelegate, UUxtGrabTargetComponent*, Grabbable, FUxtGrabPointerData, GrabPointer);
/** Delegate for handling a EndGrab event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtEndGrabDelegate, UUxtGrabTargetComponent*, Grabbable, FUxtGrabPointerData, GrabPointer);


/**
 * Interactable component that listens to pinch events from touching pointers.
 * 
 * A pointer that starts pinching while touching the actor is considered a grabbing pointer.
 * The grab is released when the pointer stops pinching, regardless of whether it is still touching or not.
 * 
 * The GrabComponent does not react to grabbing pointers by itself, but serves as a base class for manipulation.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtGrabTargetComponent : public USceneComponent, public IUxtGrabTarget
{
	GENERATED_BODY()

public:

	UUxtGrabTargetComponent();

	/** Returns true if the pointer is currently grabbing the actor.
	 * PointerData will contain the associated grab data for the pointer.
	 * Index is the order in which pointers started grabbing.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void FindGrabPointer(int32 PointerId, bool &Success, FUxtGrabPointerData &PointerData, int &Index) const;

	/** Returns the first active grab pointer.
	 * If no pointer is grabbing the Valid output will be false.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void GetPrimaryGrabPointer(bool &Valid, FUxtGrabPointerData &PointerData) const;

	/** Returns the second active grab pointer.
	 * If less than two pointers are grabbing the Valid output will be false.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void GetSecondaryGrabPointer(bool &Valid, FUxtGrabPointerData &PointerData) const;

	/** Compute the centroid of the grab points in world space. */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	FVector GetGrabPointCentroid(const FTransform &Transform) const;

	/** Compute the centroid of the pointer targets in world space. */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	FVector GetTargetCentroid() const;

	UFUNCTION(BlueprintGetter)
	bool GetTickOnlyWhileGrabbed() const;

	UFUNCTION(BlueprintSetter)
	void SetTickOnlyWhileGrabbed(bool bEnable);
	
	/** Returns a list of all currently grabbing pointers. */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	const TArray<FUxtGrabPointerData> &GetGrabPointers() const;

protected:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface

	virtual bool GetClosestGrabPoint_Implementation(const FVector& Point, FVector& OutPointOnSurface) const override;

	virtual void OnBeginGrab_Implementation(int32 PointerId, const FUxtPointerInteractionData& Data) override;
	virtual void OnUpdateGrab_Implementation(int32 PointerId, const FUxtPointerInteractionData& Data) override;
	virtual void OnEndGrab_Implementation(int32 PointerId) override;

private:

	/** Internal search function for finding active grabbing pointers */
	bool FindGrabPointerInternal(int32 PointerId, FUxtGrabPointerData const *&OutData, int &OutIndex) const;

	/** Compute the grab transform relative to the current actor world transform. */
	void ResetLocalGrabPoint(FUxtGrabPointerData &PointerData);

	void UpdateComponentTickEnabled();

public:

	/** Event raised when grab starts. */
	UPROPERTY(BlueprintAssignable)
	FUxtBeginGrabDelegate OnBeginGrab;

	/** Event raised when grab ends. */
	UPROPERTY(BlueprintAssignable)
	FUxtEndGrabDelegate OnEndGrab;

private:

	/** List of currently grabbing pointers. */
	UPROPERTY(BlueprintGetter = "GetGrabPointers", Category = "Grabbable")
	TArray<FUxtGrabPointerData> GrabPointers;

	/** If true the component tick is only enabled while the actor is being grabbed. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintGetter = "GetTickOnlyWhileGrabbed", BlueprintSetter = "SetTickOnlyWhileGrabbed", Category = "Grabbable")
	uint8 bTickOnlyWhileGrabbed : 1;
};
