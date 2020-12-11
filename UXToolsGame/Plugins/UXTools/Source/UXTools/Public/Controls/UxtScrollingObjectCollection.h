// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtBackPlateComponent.h"

#include "Controls/UxtBaseObjectCollection.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"

#include <Curves/CurveFloat.h>

#include "UxtScrollingObjectCollection.generated.h"

//
// Forward Declarations
class UBoxComponent;
class UUxtNearPointerComponent;
class UUxtFarPointerComponent;

// clang-format off

//
// Enums
UENUM(BlueprintType)
enum class EUxtScrollDirection : uint8
{
	UpAndDown			UMETA(DisplayName = "Up And Down", Tooltip = "The menu will scroll vertically. In this case Tiers will specify the number of rows, and Viewable area will specify the number of columns that are viewable. The collection will populate by filling columns in order."),
	LeftAndRight		UMETA(DisplayName = "Left And Right", Tooltip = "The menu will scroll horizontally. In this case Tiers will specify the number of columns, and Viewable area will specify the number of rows that are viewable. The collection will populate by filling rows in order."),
};

UENUM(BlueprintType)
enum class EUxtPaginateResult : uint8
{
	Success							UMETA(DisplayName = "Success", Tooltip = "The operation completed successfuly."),
	Failed_ConcurrentOperation		UMETA(DisplayName = "Failed, Concurrent Operation", Tooltip = "The operation was aborted due to another move request that has not yet finished."),
	Failed_ConcurrentInteraction	UMETA(DisplayName = "Failed, Concurrent User Interaction", Tooltip = "The operation was aborted because there is another interaction with the object." ),
};

// clang-format on

UENUM(meta = (Bitflags))
enum class EInteractionTypeBits
{
	NearInteraction,
	FarInteraction,
	TouchInteraction,
};

//
// Types

USTRUCT(BlueprintType, NoExport)
struct FScrollingCollectionProperties
{
	/** The Center of the viewable area, relative to the Scrolling Collection Component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UxtScrollingCollectionProperties - Experimental")
	FVector Center;

	/** The width of the viewable area */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UxtScrollingCollectionProperties - Experimental")
	float Width;

	/** The height of the viewable area */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UxtScrollingCollectionProperties - Experimental")
	float Height;
};

//
// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtScrollingObjectCollectionUpdated, FScrollingCollectionProperties const&, Properties);
DECLARE_DYNAMIC_DELEGATE_OneParam(FUxtScrollingObjectCollectionOnPaginationEnd, EUxtPaginateResult, Result);

/**
 * Component that adds a scrollable object menu to the actor to which it is attached
 */
UCLASS(ClassGroup = ("UXTools - Experimental"), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtScrollingObjectCollection
	: public UUxtBaseObjectCollection
	, public IUxtPokeTarget
	, public IUxtPokeHandler
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	/** Sets the number of columns or rows in respect to ViewableArea and ScrollDirection.*/
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection - Experimental")
	void SetTiers(int32 IncomingTiers);

	/** Move the collection up or down (or left or right) in multiples of #ViewableArea. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection - Experimental", meta = (AutoCreateRefTerm = "Callback"))
	void PageBy(const int32 NumPages, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& Callback);

	/** Move the collection up or down (or left or right) in multiples of #ViewableArea. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection - Experimental", meta = (AutoCreateRefTerm = "Callback"))
	void MoveByItems(const int32 NumItems, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& Callback);

	/** Adds an actor to the collection at runtime.  */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection - Experimental")
	void AddActorToCollection(AActor* ActorToAdd);

	/** Return current scroll direction */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection - Experimental", meta = (AutoCreateRefTerm = "Callback"))
	EUxtScrollDirection GetScrollDirection();

	/**  Sets default values for this component's properties */
	UUxtScrollingObjectCollection();

	/** Enables/disables interaction for specific interaction types. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (Bitmask, BitmaskEnum = "EInteractionTypeBits"))
	int32 CanScroll;

	/** The direction in which the collection will scroll. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental")
	EUxtScrollDirection ScrollDirection;

	/** Width of cell per object. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0"))
	float CellWidth;

	/** Height of cell per object. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0"))
	float CellHeight;

	/** Number of lines visible in scroller. Orthogonal to #Tiers */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental")
	int32 ViewableArea;

	/** Collision profile used by the box collider */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental")
	FName CollisionProfile;

	/** Determines whether a near scroll gesture is released when the engaged fingertip is dragged outside of the viewable area. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental")
	bool bReleaseAtScrollBoundary;

	/** Smoothing applied to direct interaction when scrolling.
	 * Note: This is currently frame rate dependent, WIP to adjust for frame rate. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0", UIMax = "1.0"))
	float ScrollSmoothing;

	/** Velocity damping factor.
	 * For no velocity to be retained set this value to 1.0.
	 * Note: This is currently applied per frame and is frame rate dependent, WIP to adjust for frame rate. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0", UIMax = "1.0"))
	float VelocityDamping;

	/** Scale the bounce return velocity. The larger this value is the quicker the collection will return. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0"))
	float BounceSpringFactor;

	/** Animation curve for pagination. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental")
	FRuntimeFloatCurve PaginationCurve;

	/** Strength of the snap to effect.
	 *	A value of 1.0 will result in instant snap to the closest item.
	 *	A value of 0.0 will disable the snap effect.
	 *	Note also that VelocityDamping will affect the feel of the snap. When damping is low or zero
	 *	the snap will be more prominent and the boundary between adjacent cells will feel sharper. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0", UIMax = "1.0"))
	float SnapToStrength;

	/** If the interaction ends before moving this far it will be considered a click. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "0.0"))
	float ClickMovementThreshold;

	/** Event raised whenever the collection is updated. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Scrolling Object Collection - Experimental")
	FUxtScrollingObjectCollectionUpdated OnCollectionUpdated;

	/** Set the backplate for this component. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection - Experimental")
	void SetBackPlate(UUxtBackPlateComponent* Plate) { BackPlate = Plate; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Called when component is destroyed
	virtual void DestroyComponent(bool bPromoteToChildred) override;

	//
	// IUxtPokeTarget interface
	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;
	virtual EUxtPokeBehaviour GetPokeBehaviour_Implementation() const override;
	virtual bool GetClosestPoint_Implementation(
		const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const override;

	//
	// IUxtPokeHandler interface
	virtual bool CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:
	/** Called to reset the visibility of all attached actors based on current collection state. */
	void ResetCollectionVisibility();

	/** Called to update the collection based on the current properties. */
	void InitializeCollection();

	/** Called to configure the box component based on the collection's properties. */
	void ConfigureBoxComponent();

	/** Helper function to transform a world space location to local space. */
	FVector LocationWorldToLocal(const FVector WorldSpaceLocation) const;

	/** Calculate the offset of a local space position along the scroll direction. */
	float GetScrollOffsetFromLocation(const FVector LocalSpaceLocation) const;

	/** Calculate the offset of a local space position along the scroll direction. */
	float GetTierOffsetFromLocation(const FVector LocalSpaceLocation) const;

	/** Tick the Collection Offset */
	void TickCollectionOffset(const float DeltaTime);

	/** Calculate the number of rows in the collection given current properties. */
	int GetNumberOfRowsInCollection() const;

	/** Get the offset from one cell to the next, taking scroll direction into account. */
	float GetSingleCellOffset() const;

	/** Is the currently an active interaction from any of the possible methods. */
	bool IsActiveInteraction() const;

	/** Verify and evaluate the pagination curve.
	 *	@retval Returns true if it is possible to evaluate the curve at the time between first and last keyframes.
	 */
	bool VerifyAndEvaluatePaginationCurve(const float EvalTime, float* const Output) const;

	/** Get the net offset which is the sum of Offset, InteractionOffset, and PaginationOffset. */
	float GetCurrentNetOffset() const;

	/** Get the current min and max valid offset. */
	void GetValidOffsetRange(float* const Minimum, float* const Maximum) const;

	/** Take interaction point and turn this into a collection item target */
	int ConvertWorldSpacePositionToButtonIndex(FVector WorldSpaceLocation);

	/** Callback to determine if we have passed movement threshold for a scroll or if we should treat the action as a click */
	UFUNCTION(Category = "Uxt Scrolling Object Collection - Experimental")
	void CheckScrollOrClick();

	/** Callback to determine if we have passed movement threshold for a scroll or if we should treat the action as a click - but for far
	 * pointer */
	UFUNCTION(Category = "Uxt Scrolling Object Collection - Experimental")
	void CheckScrollOrClickFarPointer();
	/** Current Poke Target we are passing messages to */

	UPROPERTY()
	TScriptInterface<IUxtPokeTarget> PokeTarget;

	/** Current Far Target we are passing messages to */
	UPROPERTY()
	TScriptInterface<IUxtFarTarget> FarTarget;

#if WITH_EDITORONLY_DATA

	/** Called when a property in the inspector is edited */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITORONLY_DATA

	/** Number of columns or rows in respect to ViewableArea and ScrollDirection. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental", meta = (UIMin = "1"))
	int32 Tiers;

	/** Blue BackPlate  */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection - Experimental")
	UUxtBackPlateComponent* BackPlate;

	/** Collision volume used for determining poke events. Created in BeginPlay */
	UBoxComponent* BoxComponent;

	/** Cached extent from the last time the BoxComponent's extents were calculated. Used for #bReleaseAtScrollBoundary workaround. */
	FVector BoxComponentExtents;

	/** Actor used to organize attached actors and make movement easier. Created in BeginPlay */
	USceneComponent* CollectionRoot;

	/** If not null, near pointer that is currently the focus of any interaction. */
	TWeakObjectPtr<UUxtNearPointerComponent> PokePointer;

	/** If not null, far pointer that is currently the focus of any interaction. */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointer;

	/** Local space offset along the direction of scroll at the start of the interaction. */
	float InteractionOrigin;

	/** Last local space offset along the direction of scroll, relative to #InteractionOrigin */
	float InteractionOffset;

	/** The previous interaction offset. Used to update #InteractionVelocityOffset. */
	float PrevInteractionOffset;

	/** Offset in the direction of scroll, does not include any current interaction contribution. */
	float Offset;

	/** Current velocity of the offset. */
	float OffsetVelocity;

	/** If non zero there has been a pagination request. Pagination is treated as a modal operation that blocks all other interaction. */
	float PaginationDelta;

	/** The current offset resulting from any ongoing pagination. Always applied but non zero only if #PaginationDelta is non zero. */
	float PaginationOffset;

	/** The current time in the pagination. The pagination progress is determined by using this value to evaluate #PaginationCurve */
	float PaginationTime;

	/** Pagination complete delegate. */
	FUxtScrollingObjectCollectionOnPaginationEnd OnPaginationComplete;

	/** Has hit the #ClickMovementThreshold */
	bool bHasHitClickMovementThreshold;

	/** Time we wait for a scroll before passing the click action onto the button */
	float ScrollOrClickTime;
	/** Handle for the callback to check if we are scrolling or clicking */
	FTimerHandle ScrollOrClickHandle;

#if WITH_EDITORONLY_DATA
	/** Record whether the collection has been initialized at least once while in the editor. */
	bool bCollectionInitializedInEditor;
#endif // WITH_EDITORONLY_DATA
};

/**
 *
 */
inline float UUxtScrollingObjectCollection::GetCurrentNetOffset() const
{
	return Offset + InteractionOffset + PaginationOffset;
}

/**
 *
 */
inline FVector UUxtScrollingObjectCollection::LocationWorldToLocal(const FVector WorldSpaceLocation) const
{
	return GetComponentTransform().Inverse().TransformPosition(WorldSpaceLocation);
}
