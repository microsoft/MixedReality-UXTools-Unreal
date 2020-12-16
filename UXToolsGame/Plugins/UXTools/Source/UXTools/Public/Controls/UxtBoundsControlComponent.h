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
class UBoxComponent;
class UxtConstraintManager;
struct UxtAffordanceInteractionCache;

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
UCLASS(Blueprintable, ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtBoundsControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUxtBoundsControlComponent();

	// Those two are necessary to be able to forward declare types used in the TUniquePtr template. See comment in
	// UniquePtr.h:TDefaultDelete::operator() for further details.
	UUxtBoundsControlComponent(FVTableHelper& Helper);
	~UUxtBoundsControlComponent();

	UFUNCTION(BlueprintGetter, Category = "Uxt Bounds Control|Affordances")
	AActor* GetBoundsControlActor() const;

	/** Get the map between the affordance actors and their information. */
	const TMap<UPrimitiveComponent*, FUxtAffordanceInstance>& GetPrimitiveAffordanceMap();

	UFUNCTION(BlueprintGetter, Category = "Uxt Bounds Control|Affordances")
	bool GetInitBoundsFromActor() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Bounds Control")
	const FBox& GetBounds() const;

	/** Mesh for the given kind of affordance. */
	UFUNCTION(BlueprintPure, Category = "Uxt Bounds Control|Affordances")
	UStaticMesh* GetAffordanceKindMesh(EUxtAffordanceKind Kind) const;

	/** Compute the bounding box based on the components of the bounding box actor. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Bounds Control")
	void ComputeBoundsFromComponents();

public:
	/** Configuration of the bounds control affordances. */
	UPROPERTY(EditAnywhere, Category = "Uxt Bounds Control")
	UUxtBoundsControlConfig* Config;

	/** Mesh used for a center affordance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Affordances")
	UStaticMesh* CenterAffordanceMesh;

	/** Mesh used for a face affordances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Affordances")
	UStaticMesh* FaceAffordanceMesh;

	/** Mesh used for a edge affordances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Affordances")
	UStaticMesh* EdgeAffordanceMesh;

	/** Mesh used for a corner affordances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Affordances")
	UStaticMesh* CornerAffordanceMesh;

	/** Collision box that prevents pointer rays from passing through bounds control's box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control")
	UBoxComponent* CollisionBox;

	/** The collision profile used by @ref CollisionBox. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control", AdvancedDisplay)
	FName CollisionProfile = TEXT("UI");

	/** Hand distance at which affordances become visible. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control")
	float AffordanceVisibilityDistance = 10.f;

	/** Duration of animated affordance transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control")
	float AffordanceTransitionDuration = 0.25f;

	/** Event raised when a manipulation is started. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Bounds Control")
	FUxtBoundsControlManipulationStartedDelegate OnManipulationStarted;

	/** Event raised when a manipulation is ended. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Bounds Control")
	FUxtBoundsControlManipulationEndedDelegate OnManipulationEnded;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Callback when an affordance is entering focus. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceEnterFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer);
	/** Callback when an affordance is entering focus. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceEnterGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer);
	/** Callback when an affordance is exiting focus. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceExitFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer);
	/** Callback when an affordance is exiting focus. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceExitGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer);

	/** Callback when an affordance is being grabbed. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being grabbed. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);
	/** Callback when an affordance is being released. */
	UFUNCTION(Category = "Uxt Bounds Control|Affordances")
	void OnAffordanceEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer);

	/** Callback when the parent actor is moved. */
	void OnActorTransformUpdate(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/**
	 * Look up the grab pointer data for an affordance.
	 * Returns null if the affordance is not currently grabbed.
	 */
	const FUxtGrabPointerData* FindGrabPointer(const FUxtAffordanceInstance* AffordanceInstance);

	/** Modify the target based on the current affordance interaction */
	void TransformTarget(const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer) const;

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
	/** Setup the @ref CollisionBox component. */
	void CreateCollisionBox();

	/**
	 * Resets the Transform that the @ref ConstraintsManager uses as reference.
	 *
	 * Should be called after @ref UpdateInteractionCache, so it uses the latest data.
	 */
	void ResetConstraintsReferenceTransform();

	/**
	 * Resets the data cache to be used during the interaction.
	 *
	 * This is meant to be called at the start of each new interaction.
	 */
	void UpdateInteractionCache(const FUxtAffordanceInstance* const AffordanceInstance, const FUxtGrabPointerData& GrabPointerData);

	TUniquePtr<UxtConstraintManager> ConstraintManager;

	/** Initialize bounds from actor content. */
	UPROPERTY(EditAnywhere, Category = "Uxt Bounds Control", BlueprintGetter = "GetInitBoundsFromActor")
	bool bInitBoundsFromActor = true;

	/** Current bounding box in the local space of the actor. */
	UPROPERTY(Transient, Category = "Uxt Bounds Control", BlueprintGetter = "GetBounds")
	FBox Bounds;

	/** Parameter collection used to store the finger tip position */
	UPROPERTY(Transient)
	UMaterialParameterCollection* ParameterCollection;

	/** Actor that contains affordances at runtime. */
	UPROPERTY(Transient, DuplicateTransient, Category = "Uxt Bounds Control", BlueprintGetter = "GetBoundsControlActor")
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

	/** Cache that holds certain data that is relevant during the whole interaction with an affordance. */
	TUniquePtr<UxtAffordanceInteractionCache> InteractionCache;
};
