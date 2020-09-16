// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Controls/UxtBoundsControlConfig.h"
#include "Interactions/UxtGrabTargetComponent.h"

#include "UxtBoundsControlComponent.generated.h"

class UUxtBoundsControlComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtBoundsControlManipulationStartedDelegate, UUxtBoundsControlComponent*, Manipulator, const FUxtAffordanceConfig&, AffordanceInfo,
	UUxtGrabTargetComponent*, GrabbedComponent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtBoundsControlManipulationEndedDelegate, UUxtBoundsControlComponent*, Manipulator, const FUxtAffordanceConfig&, AffordanceInfo,
	UUxtGrabTargetComponent*, GrabbedComponent);

/**
 * Manages a set of affordances that can be manipulated for changing the actor transform.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtBoundsControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUxtBoundsControlComponent();

	/** Get the map between the affordance actors and their information. */
	const TMap<AActor*, const FUxtAffordanceConfig*>& GetActorAffordanceMap();

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	TSubclassOf<class AActor> GetCenterAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	TSubclassOf<class AActor> GetFaceAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	TSubclassOf<class AActor> GetEdgeAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	TSubclassOf<class AActor> GetCornerAffordanceClass() const;

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	bool GetInitBoundsFromActor() const;

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	const FBox& GetBounds() const;

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	float GetMaximumBoundsScale() const;

	UFUNCTION(BlueprintSetter, Category = BoundsControl)
	void SetMaximumBoundsScale(float Value);

	UFUNCTION(BlueprintGetter, Category = BoundsControl)
	float GetMinimumBoundsScale() const;

	UFUNCTION(BlueprintSetter, Category = BoundsControl)
	void SetMinimumBoundsScale(float Value);

	/** Actor class that will be instantiated for the given kind of affordance. */
	UFUNCTION(BlueprintPure, Category = BoundsControl)
	TSubclassOf<class AActor> GetAffordanceKindActorClass(EUxtAffordanceKind Kind) const;

	/** Compute the bounding box based on the components of the bounding box actor. */
	UFUNCTION(BlueprintCallable, Category = BoundsControl)
	void ComputeBoundsFromComponents();

public:
	UPROPERTY(EditAnywhere, Category = BoundsControl)
	UUxtBoundsControlConfig* Config;

	/** Event raised when a manipulation is started. */
	UPROPERTY(BlueprintAssignable, Category = BoundsControl)
	FUxtBoundsControlManipulationStartedDelegate OnManipulationStarted;

	/** Event raised when a manipulation is ended. */
	UPROPERTY(BlueprintAssignable, Category = BoundsControl)
	FUxtBoundsControlManipulationEndedDelegate OnManipulationEnded;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Callback when an affordance is being grabbed. */
	UFUNCTION()
	void OnPointerBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being grabbed. */
	UFUNCTION()
	void OnPointerUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being released. */
	UFUNCTION()
	void OnPointerEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	/**
	 * Try to activate the given grab pointer on the bounding box.
	 * Returns true when the grab activation was successful and the pointer will update the bounding box.
	 */
	bool TryActivateGrabPointer(const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer);
	/**
	 * Release the grab pointer.
	 * Returns true if the pointer was grabbing and has been released.
	 */
	bool TryReleaseGrabPointer(const FUxtAffordanceConfig& Affordance);
	/**
	 * Look up the grab pointer data for an affordance.
	 * Returns null if the affordance is not currently grabbed.
	 */
	FUxtGrabPointerData* FindGrabPointer(const FUxtAffordanceConfig& Affordance);

	/** Compute new bounding box and rotation based on the currently active grab pointers. */
	void ComputeModifiedBounds(
		const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer, FBox& OutBounds, FQuat& OutDeltaRotation) const;

	/** Update the world transforms of affordance actors to match the current bounding box. */
	void UpdateAffordanceTransforms();

	/**
	 * Compute the relative translation and scale between two boxes.
	 * Returns false if relative scale can not be computed.
	 */
	static bool GetRelativeBoxTransform(const FBox& Box, const FBox& RelativeTo, FTransform& OutTransform);

private:
	/** Actor class to instantiate for a center affordance. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetCenterAffordanceClass", Category = BoundsControl)
	TSubclassOf<class AActor> CenterAffordanceClass;

	/** Actor class to instantiate for a face affordances. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetFaceAffordanceClass", Category = BoundsControl)
	TSubclassOf<class AActor> FaceAffordanceClass;

	/** Actor class to instantiate for a edge affordances. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetEdgeAffordanceClass", Category = BoundsControl)
	TSubclassOf<class AActor> EdgeAffordanceClass;

	/** Actor class to instantiate for a corner affordances. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetCornerAffordanceClass", Category = BoundsControl)
	TSubclassOf<class AActor> CornerAffordanceClass;

	/** Initialize bounds from actor content. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetInitBoundsFromActor", Category = BoundsControl)
	bool bInitBoundsFromActor = true;

	/** Current bounding box in the local space of the actor. */
	UPROPERTY(Transient, BlueprintGetter = "GetBounds", Category = BoundsControl)
	FBox Bounds;

	/**
	 * Maps actors to the affordances they represent.
	 * This is used for looking up the correct affordance settings when grab events are handled.
	 */
	TMap<AActor*, const FUxtAffordanceConfig*> ActorAffordanceMap;

	/**
	 * Contains the currently active affordances being moved by grab pointers.
	 *
	 * Note:
	 * Currently should only ever contain a single grab pointer.
	 * In the future multiple simultaneous grabs may be supported.
	 */
	TArray<TPair<const FUxtAffordanceConfig*, FUxtGrabPointerData>> ActiveAffordanceGrabPointers;

	/** Initial bounding box at the start of interaction. */
	FBox InitialBounds;
	/** Initial transform at the start of interaction. */
	FTransform InitialTransform;

	/** Minimum valid scale for bounds. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetMinimumBoundsScale", BlueprintSetter = "SetMinimumBoundsScale", Category = BoundsControl)
	float MinimumBoundsScale;

	/** Maximum valid scale for bounds. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetMaximumBoundsScale", BlueprintSetter = "SetMaximumBoundsScale", Category = BoundsControl)
	float MaximumBoundsScale;
};
