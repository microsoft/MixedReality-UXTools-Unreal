// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InteractableComponent.h"

#include "GrabbableComponent.generated.h"


class UTouchPointer;

/**
 * Utility struct that stores transient data for a pointer which is interacting with a grabbable component.
 */
USTRUCT(BlueprintType)
struct MIXEDREALITYUTILS_API FGrabPointerData
{
	GENERATED_BODY()

	/** The pointer that is interacting with the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grab Pointer Data")
	UTouchPointer *Pointer;

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
class MIXEDREALITYUTILS_API UGrabPointerDataFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Compute the grab point in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetGrabLocation(const FTransform &Transform, const FGrabPointerData &PointerData);

	/** Compute the grab rotation in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetGrabRotation(const FTransform &Transform, const FGrabPointerData &PointerData);

	/** Compute the grab transform in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FTransform GetGrabTransform(const FTransform &Transform, const FGrabPointerData &PointerData);

	/** Compute the pointer target in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetTargetLocation(const FGrabPointerData &PointerData);

	/** Compute the target rotation in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetTargetRotation(const FGrabPointerData &PointerData);

	/** Compute the pointer target transform in world space. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FTransform GetTargetTransform(const FGrabPointerData &PointerData);

	/** Compute the world space offset between pointer grab point and target. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FVector GetLocationOffset(const FTransform &Transform, const FGrabPointerData &PointerData);

	/** Compute the world space rotation between pointer grab point and target. */
	UFUNCTION(BlueprintPure, Category = "GrabPointer")
	static FRotator GetRotationOffset(const FTransform &Transform, const FGrabPointerData &PointerData);
};


/** Delegate for handling a BeginGrab event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBeginGrabDelegate, UGrabbableComponent*, Grabbable, FGrabPointerData, GrabPointer);
/** Delegate for handling a EndGrab event. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEndGrabDelegate, UGrabbableComponent*, Grabbable, FGrabPointerData, GrabPointer);


/**
 * Interactable component that listens to pinch events from touching pointers.
 * 
 * A pointer that starts pinching while touching the actor is considered a grabbing pointer.
 * The grab is released when the pointer stops pinching, regardless of whether it is still touching or not.
 * 
 * The GrabbableComponent does not react to grabbing pointers by itself, but serves as a base class for manipulation.
 */
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MIXEDREALITYUTILS_API UGrabbableComponent : public UInteractableComponent
{
	GENERATED_BODY()

public:

	UGrabbableComponent();

	/** Returns true if the pointer is currently grabbing the actor.
	 * PointerData will contain the associated grab data for the pointer.
	 * Index is the order in which pointers started grabbing.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void FindGrabPointer(UTouchPointer *Pointer, bool &Success, FGrabPointerData &PointerData, int &Index) const;

	/** Returns the first active grab pointer.
	 * If no pointer is grabbing the Valid output will be false.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void GetPrimaryGrabPointer(bool &Valid, FGrabPointerData &PointerData) const;

	/** Returns the second active grab pointer.
	 * If less than two pointers are grabbing the Valid output will be false.
	 */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	void GetSecondaryGrabPointer(bool &Valid, FGrabPointerData &PointerData) const;

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

protected:

	//
	// ITouchPointerTarget interface

	virtual void GraspStarted_Implementation(UTouchPointer* Pointer) override;
	virtual void GraspEnded_Implementation(UTouchPointer* Pointer) override;

	/** Returns a list of all currently grabbing pointers. */
	UFUNCTION(BlueprintPure, Category = "Grabbable")
	const TArray<FGrabPointerData> &GetGrabPointers() const;

private:

	/** Internal search function for finding active grabbing pointers */
	bool FindGrabPointerInternal(UTouchPointer *Pointer, FGrabPointerData const *&OutData, int &OutIndex) const;

	/** Compute the grab transform relative to the current actor world transform. */
	void ResetLocalGrabPoint(FGrabPointerData &PointerData);

	void UpdateComponentTickEnabled();

public:

	/** Event raised when grab starts. */
	UPROPERTY(BlueprintAssignable)
	FBeginGrabDelegate OnBeginGrab;

	/** Event raised when grab ends. */
	UPROPERTY(BlueprintAssignable)
	FEndGrabDelegate OnEndGrab;

private:

	/** List of currently grabbing pointers. */
	UPROPERTY(BlueprintGetter = "GetGrabPointers", Category = "Grabbable")
	TArray<FGrabPointerData> GrabPointers;

	/** If true the component tick is only enabled while the actor is being grabbed. */
	UPROPERTY(BlueprintGetter = "GetTickOnlyWhileGrabbed", BlueprintSetter = "SetTickOnlyWhileGrabbed")
	bool bTickOnlyWhileGrabbed = true;

};
