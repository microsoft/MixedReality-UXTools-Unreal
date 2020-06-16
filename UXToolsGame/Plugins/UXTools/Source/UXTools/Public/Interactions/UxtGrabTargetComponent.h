// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/SceneComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtManipulationFlags.h"

#include "UxtGrabTargetComponent.generated.h"

/**
 * Utility struct that stores transient data for a pointer which is interacting with a grabbable component.
 */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtGrabPointerData
{
	GENERATED_BODY()

	/** The near pointer that is interacting with the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	UUxtNearPointerComponent* NearPointer = nullptr;

	/** The far pointer that is interacting with the component */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	UUxtFarPointerComponent* FarPointer = nullptr;

	/** Last updated grab point transform. (Pointer transform in near pointer case, ray hit transform in far pointer case) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	FTransform GrabPointTransform;

	/** The time at which interaction started, in seconds since application start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	float StartTime;

	/**
	 * Transform of the pointer when it started interacting, in the local space of the target component.
	 * This allows computing pointer offset in relation to the current actor transform.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	FTransform LocalGrabPoint;

	/** Far pointer only property -> describes the relative transform of the grab point to the pointer transform (pointer origin / orientation)
	  * This is needed to calculate the new grab point on the object on pointer translations / rotations */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	FTransform FarRayHitPointInPointer = FTransform::Identity;
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

	/** Compute the grab point transform in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FTransform GetGrabPointTransform(const FUxtGrabPointerData& GrabData);

	/** Compute the world space offset between pointer grab point and target. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetLocationOffset(const FTransform &Transform, const FUxtGrabPointerData& GrabData);

	/** Compute the world space rotation between pointer grab point and target. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetRotationOffset(const FTransform &Transform, const FUxtGrabPointerData& GrabData);

	/** Returns the world space pointer transform (at pointer origin). */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FTransform GetPointerTransform(const FUxtGrabPointerData& GrabData);

	/** Returns the world space pointer location */
	UFUNCTION(BluePrintPure, Category = "GrabPointer")
	static FVector GetPointerLocation(const FUxtGrabPointerData& GrabData);
};


/** Delegate for handling a focus enter events. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtEnterGrabFocusDelegate, UUxtGrabTargetComponent*, Grabbable, UUxtNearPointerComponent*, Pointer);
/** Delegate for handling a focus update event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtUpdateGrabFocusDelegate, UUxtGrabTargetComponent*, Grabbable, UUxtNearPointerComponent*, Pointer);
/** Delegate for handling a focus exit event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtExitGrabFocusDelegate, UUxtGrabTargetComponent*, Grabbable, UUxtNearPointerComponent*, Pointer);

/** Delegate for handling a BeginGrab event. Grabbing pointer is added to the object before this is triggered. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtBeginGrabDelegate, UUxtGrabTargetComponent*, Grabbable, FUxtGrabPointerData, GrabPointer);
/** Delegate for handling a UpdateGrab event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtUpdateGrabDelegate, UUxtGrabTargetComponent*, Grabbable, FUxtGrabPointerData, GrabPointer);
/** Delegate for handling a EndGrab event. Grabbing pointer is removed from the object before this is triggered. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtEndGrabDelegate, UUxtGrabTargetComponent*, Grabbable, FUxtGrabPointerData, GrabPointer);


/**
 * Interactable component that listens to grab events from near pointers.
 * 
 * A pointer that starts grabing while near the actor is considered a grabbing pointer.
 * The grab is released when the pointer stops grabing, regardless of whether it is still near or not.
 * 
 * The GrabComponent does not react to grabbing pointers by itself, but serves as a base class for manipulation.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtGrabTargetComponent : public USceneComponent, public IUxtGrabTarget, public IUxtFarTarget
{
	GENERATED_BODY()

public:

	UUxtGrabTargetComponent();

	/** Returns true if the pointer is currently grabbing the actor.
	 * PointerData will contain the associated grab data for the pointer.
	 * Index is the order in which pointers started grabbing.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void FindGrabPointer(UUxtNearPointerComponent* NearPointer, UUxtFarPointerComponent* FarPointer, bool &Success, FUxtGrabPointerData &PointerData, int &Index) const;

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

	/**
	 * Release all currently grabbing pointers.
	 * Returns true if any pointers were grabbing and have been released, false if no pointers were grabbing.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grabbable")
	bool ForceEndGrab();

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
	const TArray<FUxtGrabPointerData>& GetGrabPointers() const;

protected:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface
	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer) override;

	/** Compute the average transform of currently grabbing pointers */
	FTransform GetGrabPointCentroidTransform() const;
private:

	/** Internal search function for finding active grabbing pointers */
	bool FindGrabPointerInternal(UUxtNearPointerComponent* NearPointer, UUxtFarPointerComponent* FarPointer, FUxtGrabPointerData const *&OutData, int &OutIndex) const;

	/** Compute the grab transform relative to the current actor world transform. */
	void ResetLocalGrabPoint(FUxtGrabPointerData &PointerData);

	void UpdateComponentTickEnabled();

	void InitGrabTransform(FUxtGrabPointerData& GrabData) const;

public:

	/** Event raised when entering grab focus. */
	UPROPERTY(BlueprintAssignable)
	FUxtEnterGrabFocusDelegate OnEnterGrabFocus;

	/** Event raised when grab focus updates. */
	UPROPERTY(BlueprintAssignable)
	FUxtUpdateGrabFocusDelegate OnUpdateGrabFocus;

	/** Event raised when exiting grab. */
	UPROPERTY(BlueprintAssignable)
	FUxtExitGrabFocusDelegate OnExitGrabFocus;

	/** Event raised when grab starts. */
	UPROPERTY(BlueprintAssignable)
	FUxtBeginGrabDelegate OnBeginGrab;

	/** Event raised when grab updates. */
	UPROPERTY(BlueprintAssignable)
	FUxtUpdateGrabDelegate OnUpdateGrab;

	/** Event raised when grab ends. */
	UPROPERTY(BlueprintAssignable)
	FUxtEndGrabDelegate OnEndGrab;

	/** Property that indicates if the object is grabbable with far or near interaction or both. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interaction, meta = (Bitmask, BitmaskEnum = EUxtInteractionMode))
	int32 InteractionMode;

private:

	/** List of currently grabbing pointers. */
	UPROPERTY(BlueprintGetter = "GetGrabPointers", Category = "Grabbable")
	TArray<FUxtGrabPointerData> GrabPointers;

	/** If true the component tick is only enabled while the actor is being grabbed. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintGetter = "GetTickOnlyWhileGrabbed", BlueprintSetter = "SetTickOnlyWhileGrabbed", Category = "Grabbable")
	uint8 bTickOnlyWhileGrabbed : 1;
};
