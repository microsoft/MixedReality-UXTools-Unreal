// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtBoundsControlConfig.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Interactions/UxtManipulatorComponent.h"
#include "Materials/MaterialParameterCollection.h"

#include "UxtBoundsControlComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUxtBoundsControl, Log, All);

class UUxtBoundsControlComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMesh;
class UBoxComponent;
struct UxtAffordanceInteractionCache;

/** Instance of an affordance on the bounds control actor. */
USTRUCT()
struct FUxtAffordanceInstance
{
	GENERATED_BODY()

	/** Copy of the config used for generating the affordance. */
	FUxtAffordanceConfig Config;

	/** Dynamic material for highlighting the affordance. */
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial = nullptr;

	/** Percentage of transition to the focused state. */
	float FocusedTransition = 0.0f;

	/** Percentage of transition to the grabbed state. */
	float ActiveTransition = 0.0f;

	/** Refcount of pointers currently focusing the affordance. */
	int FocusCount = 0;

	/** Initial scale, used to calculate @ref ReferenceScale */
	FVector InitialScale = FVector::OneVector;

	/** Reference scale to be used during scaling animations */
	FVector ReferenceScale = FVector::OneVector;
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
class UXTOOLS_API UUxtBoundsControlComponent : public UUxtManipulatorComponent
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
	const TMap<UPrimitiveComponent*, FUxtAffordanceInstance>& GetPrimitiveAffordanceMap() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Bounds Control|Affordances")
	bool GetInitBoundsFromActor() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Bounds Control")
	UStaticMeshComponent* GetBoundingBox() const;

	UFUNCTION(BlueprintGetter, Category = "Uxt Bounds Control")
	const FBox& GetBounds() const;

	/** Mesh for the given kind of affordance. */
	UFUNCTION(BlueprintPure, Category = "Uxt Bounds Control|Affordances")
	UStaticMesh* GetAffordanceKindMesh(EUxtAffordanceKind Kind) const;

	/** Compute the bounding box based on the components of the bounding box actor. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Bounds Control")
	void ComputeBoundsFromComponents();

	/** Returns the bounds override component or nullptr if one hasn't been set. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Bounds Control")
	USceneComponent* GetBoundsOverride() const { return Cast<USceneComponent>(BoundsOverride.GetComponent(GetOwner())); }

	/**
	 * Sets the component to create the bounds around. If not set the actor root will be used.
	 * It must be owned by the same actor as the bounds control. Pass nullptr to clear the override.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Bounds Control")
	void SetBoundsOverride(USceneComponent* NewBoundsOverride);

	/**
	 * Sets the component to create the bounds around. If not set the actor root will be used.
	 * It must be owned by the same actor as the bounds control.
	 * Use this override if the bounds override needs to be serialized, e.g. when setting
	 * from an actor constructor.
	 */
	void SetBoundsOverride(const FComponentReference& NewBoundsOverride);

	UPrimitiveComponent* GetAffordancePrimitive(const EUxtAffordancePlacement Placement) const;

	/** Retrieves the component that is considered the root for the bounds. This is @ref BoundsOverride or the owner's root. */
	USceneComponent* GetBoundsRoot() const;

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

	/** Mesh used for the bounding box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Advanced")
	UStaticMesh* BoundingBoxMesh;

	/** Material used by the bounding box by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Advanced")
	UMaterialInstance* BoundingBoxDefaultMaterial;

	/** Material used by the bounding box while grabbed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Bounds Control|Advanced")
	UMaterialInstance* BoundingBoxGrabbedMaterial;

	/** The collision profile used by @ref BoundingBoxComponent. */
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

	virtual void OnExternalManipulationStarted() override;

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

	/** Keeps affordances at the appropriate scale. */
	void UpdateAffordanceScales();

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
	/** Creates the bounding box that surrounds the actor. */
	void CreateBoundingBox();

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

	/**
	 * Handles TransformUpdated event for the component returned by @ref GetBoundsRoot.
	 */
	void OnTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/** Initializes the component at runtime */
	void Init();

	/** Deinitializes the component at runtime */
	void Deinit();

	/** Initialize bounds from actor content. */
	UPROPERTY(EditAnywhere, Category = "Uxt Bounds Control", BlueprintGetter = "GetInitBoundsFromActor")
	bool bInitBoundsFromActor = true;

	/** Component added automatically that holds the bounding box mesh used to surround the object. */
	UPROPERTY(Transient, Category = "Uxt Bounds Control", BlueprintGetter = "GetBoundingBox")
	UStaticMeshComponent* BoundingBoxComponent;

	/** Component used for bounds calculation, instead of the actor's root */
	UPROPERTY(EditAnywhere, Category = "Uxt Bounds Control", meta = (UseComponentPicker, AllowedClasses = "SceneComponent"))
	FComponentReference BoundsOverride;

	/** Current bounding box in the local space of @ref GetBoundsRoot. */
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

	/** Allows updating affordance scales only when necessary. */
	bool bAffordanceScalesNeedUpdate;

	FDelegateHandle TransformUpdatedDelegateHandle;
};
