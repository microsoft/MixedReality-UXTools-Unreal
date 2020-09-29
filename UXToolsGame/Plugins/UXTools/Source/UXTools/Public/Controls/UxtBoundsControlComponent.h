// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Controls/UxtBoundsControlConfig.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Materials/MaterialParameterCollection.h"

#include "UxtBoundsControlComponent.generated.h"

class UUxtBoundsControlComponent;
class UMaterialInstanceDynamic;

/** Instance of an affordance on the bounds control actor. */
USTRUCT()
struct FUxtAffordanceInstance
{
	GENERATED_BODY()

	/** Copy of the config used for generating the affordance. */
	UPROPERTY()
	FUxtAffordanceConfig Config;

	/** Dynamic material for highlighting the affordance. */
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	/** Percentage of transition to the focused state. */
	UPROPERTY()
	float FocusedTransition = 0.0f;

	/** Percentage of transition to the grabbed state. */
	UPROPERTY()
	float ActiveTransition = 0.0f;

	/** Refcount of pointers currently focusing the affordance. */
	UPROPERTY()
	int FocusCount = 0;
};

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
	const TMap<AActor*, FUxtAffordanceInstance>& GetActorAffordanceMap();

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

	/** Actor class that will be instantiated for the given kind of affordance. */
	UFUNCTION(BlueprintPure, Category = BoundsControl)
	TSubclassOf<class AActor> GetAffordanceKindActorClass(EUxtAffordanceKind Kind) const;

	/** Compute the bounding box based on the components of the bounding box actor. */
	UFUNCTION(BlueprintCallable, Category = BoundsControl)
	void ComputeBoundsFromComponents();

public:
	/** Configuration of the bounds control affordances. */
	UPROPERTY(EditAnywhere, Category = BoundsControl)
	UUxtBoundsControlConfig* Config;

	/** Minimum valid scale for bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	float MinimumBoundsScale = 0.1f;

	/** Maximum valid scale for bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	float MaximumBoundsScale = 5.0f;

	/** Hand distance at which affordances become visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	float AffordanceVisibilityDistance = 10.f;

	/** Duration of animated affordance transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	float AffordanceTransitionDuration = 0.25f;

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

	/** Callback when an affordance is entering focus. */
	UFUNCTION()
	void OnAffordanceEnterFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer);
	/** Callback when an affordance is entering focus. */
	UFUNCTION()
	void OnAffordanceEnterGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer);
	/** Callback when an affordance is exiting focus. */
	UFUNCTION()
	void OnAffordanceExitFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer);
	/** Callback when an affordance is exiting focus. */
	UFUNCTION()
	void OnAffordanceExitGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer);

	/** Callback when an affordance is being grabbed. */
	UFUNCTION()
	void OnAffordanceBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being grabbed. */
	UFUNCTION()
	void OnAffordanceUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being released. */
	UFUNCTION()
	void OnAffordanceEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	/** Callback when the parent actor is moved. */
	void OnActorTransformUpdate(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/**
	 * Try to activate the given grab pointer on the bounding box.
	 * Returns true when the grab activation was successful and the pointer will update the bounding box.
	 */
	bool TryActivateGrabPointer(const FUxtAffordanceInstance* AffordanceInstance, const FUxtGrabPointerData& GrabPointer);
	/**
	 * Release the grab pointer.
	 * Returns true if the pointer was grabbing and has been released.
	 */
	bool TryReleaseGrabPointer(const FUxtAffordanceInstance* AffordanceInstance);
	/**
	 * Look up the grab pointer data for an affordance.
	 * Returns null if the affordance is not currently grabbed.
	 */
	FUxtGrabPointerData* FindGrabPointer(const FUxtAffordanceInstance* AffordanceInstance);

	/** Compute new bounding box and rotation based on the currently active grab pointers. */
	void ComputeModifiedBounds(
		const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer, FBox& OutBounds, FQuat& OutDeltaRotation) const;

	/** Update the world transforms of affordance actors to match the current bounding box. */
	void UpdateAffordanceTransforms();

	/** Update animated properties such as affordance highlights. */
	void UpdateAnimation(float DeltaTime);

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

	/** Parameter collection used to store the finger tip position */
	UPROPERTY(Transient)
	UMaterialParameterCollection* ParameterCollection;

	/**
	 * Maps actors to the affordances they represent.
	 * This is used for looking up the correct affordance settings when grab events are handled.
	 */
	UPROPERTY(Transient)
	TMap<AActor*, FUxtAffordanceInstance> ActorAffordanceMap;

	/**
	 * Contains the currently active affordances being moved by grab pointers.
	 *
	 * Note:
	 * Currently should only ever contain a single grab pointer.
	 * In the future multiple simultaneous grabs may be supported.
	 */
	TArray<TPair<const FUxtAffordanceInstance*, FUxtGrabPointerData>> ActiveAffordanceGrabPointers;

	/** Initial bounding box at the start of interaction. */
	UPROPERTY(Transient)
	FBox InitialBounds;

	/** Initial transform at the start of interaction. */
	UPROPERTY(Transient)
	FTransform InitialTransform;
};
