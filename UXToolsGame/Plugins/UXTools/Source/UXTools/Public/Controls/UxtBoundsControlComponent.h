// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Controls/UxtBoundsControlConfig.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Materials/MaterialParameterCollection.h"

#include "UxtBoundsControlComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUxtBoundsControl, Log, All);

class UUxtBoundsControlComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMesh;

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

	UFUNCTION(BlueprintGetter)
	AActor* GetBoundsControlActor() const;

	/** Get the map between the affordance actors and their information. */
	const TMap<UPrimitiveComponent*, FUxtAffordanceInstance>& GetPrimitiveAffordanceMap();

	UFUNCTION(BlueprintGetter)
	bool GetInitBoundsFromActor() const;

	UFUNCTION(BlueprintGetter)
	const FBox& GetBounds() const;

	/** Mesh for the given kind of affordance. */
	UFUNCTION(BlueprintPure, Category = BoundsControl)
	UStaticMesh* GetAffordanceKindMesh(EUxtAffordanceKind Kind) const;

	/** Compute the bounding box based on the components of the bounding box actor. */
	UFUNCTION(BlueprintCallable, Category = BoundsControl)
	void ComputeBoundsFromComponents();

public:
	/** Configuration of the bounds control affordances. */
	UPROPERTY(EditAnywhere, Category = BoundsControl)
	UUxtBoundsControlConfig* Config;

	/** Mesh used for a center affordance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	UStaticMesh* CenterAffordanceMesh;

	/** Mesh used for a face affordances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	UStaticMesh* FaceAffordanceMesh;

	/** Mesh used for a edge affordances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	UStaticMesh* EdgeAffordanceMesh;

	/** Mesh used for a corner affordances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BoundsControl)
	UStaticMesh* CornerAffordanceMesh;

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
	 * Look up the grab pointer data for an affordance.
	 * Returns null if the affordance is not currently grabbed.
	 */
	const FUxtGrabPointerData* FindGrabPointer(const FUxtAffordanceInstance* AffordanceInstance);

	/** Compute new bounding box and rotation based on the currently active grab pointers. */
	void ComputeModifiedBounds(
		const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer, FBox& OutBounds, FQuat& OutDeltaRotation) const;

	/** Create the BoundsControlActor and all affordances described in the config. */
	void CreateAffordances();

	/** Destroy the BoundsControlActor and affordance instances. */
	void DestroyAffordances();

	/** Update the world transforms of affordance actors to match the current bounding box. */
	void UpdateAffordanceTransforms();

	/** Update animated properties such as affordance highlights. */
	void UpdateAffordanceAnimation(float DeltaTime);

	/** Returns true if the affordance instance is currently bing grabbed. */
	bool IsAffordanceGrabbed(const FUxtAffordanceInstance* Affordance) const;

	/**
	 * Compute the relative translation and scale between two boxes.
	 * Returns false if relative scale can not be computed.
	 */
	static bool GetRelativeBoxTransform(const FBox& Box, const FBox& RelativeTo, FTransform& OutTransform);

private:
	/** Initialize bounds from actor content. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetInitBoundsFromActor", Category = BoundsControl)
	bool bInitBoundsFromActor = true;

	/** Current bounding box in the local space of the actor. */
	UPROPERTY(Transient, BlueprintGetter = "GetBounds", Category = BoundsControl)
	FBox Bounds;

	/** Parameter collection used to store the finger tip position */
	UPROPERTY(Transient)
	UMaterialParameterCollection* ParameterCollection;

	/** Actor that contains affordances at runtime. */
	UPROPERTY(Transient, DuplicateTransient, BlueprintGetter = "GetBoundsControlActor", Category = BoundsControl)
	AActor* BoundsControlActor;

	UPROPERTY(Transient, DuplicateTransient)
	UUxtGrabTargetComponent* BoundsControlGrabbable;

	/**
	 * Maps primitives to the affordances they represent.
	 * This is used for looking up the correct affordance settings when grab events are handled.
	 */
	UPROPERTY(Transient)
	TMap<UPrimitiveComponent*, FUxtAffordanceInstance> PrimitiveAffordanceMap;

	/**
	 * Contains the currently active affordances being moved by grab pointers.
	 *
	 * Note:
	 * Currently should only ever contain a single grab pointer.
	 * In the future multiple simultaneous grabs may be supported.
	 */
	TArray<FUxtAffordanceInstance*> GrabbedAffordances;

	/** Initial bounding box at the start of interaction. */
	UPROPERTY(Transient)
	FBox InitialBounds;

	/** Initial transform at the start of interaction. */
	UPROPERTY(Transient)
	FTransform InitialTransform;
};
