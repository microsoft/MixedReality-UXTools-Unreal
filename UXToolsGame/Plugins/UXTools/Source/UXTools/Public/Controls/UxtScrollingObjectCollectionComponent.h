// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtBaseObjectCollectionComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"

#include <Curves/CurveFloat.h>

#include "UxtScrollingObjectCollectionComponent.generated.h"

//
// Forward Declarations
class UBoxComponent;
class UUxtNearPointerComponent;
class UUxtFarPointerComponent;
class UUxtPointerComponent;
class UUxtBackPlateComponent;

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
};

//
// Types

USTRUCT(BlueprintType, NoExport)
struct FScrollingCollectionScrollableBounds
{
	/** The center of the scrollable area, relative to the scrolling collection component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UxtScrollingCollectionProperties")
	FVector RelativeCenter;

	/** The depth, width, and height of the scrollable area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UxtScrollingCollectionProperties")
	FVector Extents;
};

//
// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FUxtScrollingObjectCollectionUpdated, FScrollingCollectionScrollableBounds const&, ScrollableBounds);
DECLARE_DYNAMIC_DELEGATE_OneParam(FUxtScrollingObjectCollectionOnPaginationEnd, EUxtPaginateResult, Result);

/**
 * Component that adds a scrollable object menu to the actor to which it is attached.
 */
UCLASS(ClassGroup = ("UXTools"), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtScrollingObjectCollectionComponent
	: public UUxtBaseObjectCollectionComponent
	, public IUxtPokeTarget
	, public IUxtPokeHandler
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	/**  Sets default values for this component's properties. */
	UUxtScrollingObjectCollectionComponent();

	/** Returns a bitmask of what specific interaction types are enabled. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	int32 GetCanScroll() const { return CanScroll; }

	/** Sets a bitmask of what specific interaction types are enabled. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetCanScroll(int32 Scroll) { CanScroll = Scroll; }

	/** Return current scroll direction. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	EUxtScrollDirection GetScrollDirection() const { return ScrollDirection; }

	/** Sets current scroll direction. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetScrollDirection(EUxtScrollDirection Direction);

	/** Gets if the collection has a common back plate.*/
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	bool GetIsPlated() const { return bIsPlated; }

	/** Sets if the collection has a common back plate.*/
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetIsPlated(bool IsPlated);

	/** Gets the PlatedPadding*/
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	FVector GetPlatedPadding() const { return PlatedPadding; }

	/** Sets the PlatedPadding*/
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetPlatedPadding(const FVector Padding);

	/** Gets the number of columns or rows in respect to ViewableArea and ScrollDirection.*/
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	int32 GetTiers() const { return Tiers; }

	/** Sets the number of columns or rows in respect to ViewableArea and ScrollDirection.*/
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetTiers(int32 IncomingTiers);

	/** Gets the number of columns or rows in respect to the ScrollDirection.*/
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	int32 GetViewableArea() const { return ViewableArea; }

	/** Sets the number of columns or rows in respect to the ScrollDirection.*/
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetViewableArea(int32 IncomingViewableArea);

	/** Gets the cell dimensions.*/
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	FVector GetCellSize() const { return CellSize; }

	/** Sets the cell dimensions.*/
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetCellSize(const FVector& Size);

	/** Gets the collision profile name. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Scrolling Object Collection")
	FName GetCollisionProfile() const { return CollisionProfile; }

	/** Sets the collision profile name. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Scrolling Object Collection")
	void SetCollisionProfile(FName Profile);

	/** Move the collection up or down (or left or right) in multiples of the viewable area. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection", meta = (AutoCreateRefTerm = "Callback"))
	void PageBy(const int32 NumPages, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& Callback);

	/** Move the collection up or down (or left or right) in multiples of items. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection", meta = (AutoCreateRefTerm = "Callback"))
	void MoveByItems(const int32 NumItems, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& Callback);

	/** Adds an actor to the collection at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection")
	void AddActorToCollection(AActor* ActorToAdd);

	/** Removes an actor from the collection at runtime. Optionally can destroy the actor as well. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection")
	void RemoveActorFromCollection(AActor* ActorToRemove, bool DestroyActor = false);

	/** Returns the last calculated (cached) scrollable bounds. */
	UFUNCTION(BlueprintPure, Category = "Uxt Scrolling Object Collection")
	const FScrollingCollectionScrollableBounds& GetScrollableBounds() const { return ScrollableBounds; }

	/** Event raised whenever the collection is updated. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Scrolling Object Collection")
	FUxtScrollingObjectCollectionUpdated OnCollectionUpdated;

	/** Determines whether a near scroll gesture is released when the engaged fingertip is dragged outside of the viewable area. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay)
	bool bReleaseAtScrollBoundary = false;

	/** Smoothing applied to direct interaction when scrolling.
	 * Note: This is currently frame rate dependent, WIP to adjust for frame rate. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay, meta = (UIMin = "0.0", UIMax = "1.0"))
	float ScrollSmoothing = 0.5f;

	/** Velocity damping factor.
	 * For no velocity to be retained set this value to 1.0.
	 * Note: This is currently applied per frame and is frame rate dependent, WIP to adjust for frame rate. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay, meta = (UIMin = "0.0", UIMax = "1.0"))
	float VelocityDamping = 0.04f;

	/** Scale the bounce return velocity. The larger this value is the quicker the collection will return. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay, meta = (UIMin = "0.0"))
	float BounceSpringFactor = 0.75f;

	/** Animation curve for pagination. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay)
	FRuntimeFloatCurve PaginationCurve;

	/** Strength of the snap to effect.
	 *	A value of 1.0 will result in instant snap to the closest item.
	 *	A value of 0.0 will disable the snap effect.
	 *	Note also that VelocityDamping will affect the feel of the snap. When damping is low or zero
	 *	the snap will be more prominent and the boundary between adjacent cells will feel sharper. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay, meta = (UIMin = "0.0", UIMax = "1.0"))
	float SnapToStrength = 0.0f;

	/** If the interaction ends before moving this far (in Unreal units) it will be considered a click. */
	UPROPERTY(EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay, meta = (UIMin = "0.0"))
	float ClickMovementThreshold = 1.0f;

protected:
	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void DestroyComponent(bool bPromoteToChildred) override;

	//
	// USceneComponent interface

	/** Updates the clipping box when the transform changes. */
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

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

	//
	// UUxtBaseObjectCollection interface

	/** Called to update the collection based on the current properties. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Scrolling Object Collection", CallInEditor)
	virtual void RefreshCollection() override;

	/** Component used to contain attached actors and make movement easier. */
	UPROPERTY(Transient)
	USceneComponent* CollectionRootComponent = nullptr;

	/** Collision volume used for determining poke events. */
	UPROPERTY(Transient)
	UBoxComponent* BoxComponent = nullptr;

	/** Back plate mesh component. */
	UPROPERTY(Transient)
	UUxtBackPlateComponent* BackPlateMeshComponent = nullptr;

private:
#if WITH_EDITORONLY_DATA
	/** Called when a property in the inspector is edited. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITORONLY_DATA

	/** Called to reset the visibility of all attached actors based on current collection state. */
	void ResetCollectionVisibility();

	/** Called to pass the latest clipping box state into all primitive components within the collection. */
	void UpdateClippingBox();

	/** Called to configure the back plate component based on the collection's properties. */
	void ConfigureBackPlate();

	/** Called to configure the box component based on the collection's properties. */
	void ConfigureBoxComponent();

	/** Helper function to transform a world space location to local space. */
	FVector LocationWorldToLocal(const FVector WorldSpaceLocation) const
	{
		return GetComponentTransform().Inverse().TransformPosition(WorldSpaceLocation);
	}

	/** Calculate the offset of a local space position along the scroll direction. */
	float GetScrollOffsetFromLocation(const FVector LocalSpaceLocation) const;

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
	float GetCurrentNetOffset() const { return Offset + InteractionOffset + PaginationOffset; }

	/** Get the current min and max valid offset. */
	void GetValidOffsetRange(float* const Minimum, float* const Maximum) const;

	/** Callback to determine if we have passed movement threshold for a scroll or if we should treat the action as a click for near
	 * pointer */
	UFUNCTION(Category = "Uxt Scrolling Object Collection")
	void CheckScrollOrClickNearPointer();

	/** Callback to determine if we have passed movement threshold for a scroll or if we should treat the action as a click for far
	 * pointer */
	UFUNCTION(Category = "Uxt Scrolling Object Collection")
	void CheckScrollOrClickFarPointer();

	/** Common implementation between CheckScrollOrClick and CheckScrollOrClickFarPointer. */
	AActor* CheckScrollOrClickCommon(const UUxtPointerComponent* Pointer, const FVector& WorldSpaceLocation);

	/** Returns true if the actor is in the collection, or a descendant of an actor in the collection. */
	bool IsInCollection(const AActor* Actor) const;

	/** Enables/disables interaction for specific interaction types. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|General", BlueprintGetter = "GetCanScroll",
		BlueprintSetter = "SetCanScroll", meta = (Bitmask, BitmaskEnum = "EInteractionTypeBits"))
	int32 CanScroll =
		1 << static_cast<int32>(EInteractionTypeBits::NearInteraction) | 1 << static_cast<int32>(EInteractionTypeBits::FarInteraction);

	/** The direction in which the collection will scroll. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|General", BlueprintGetter = "GetScrollDirection",
		BlueprintSetter = "SetScrollDirection")
	EUxtScrollDirection ScrollDirection = EUxtScrollDirection::UpAndDown;

	/** True if the collection should display a common back plate. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|General", BlueprintGetter = "GetIsPlated",
		BlueprintSetter = "SetIsPlated")
	bool bIsPlated = true;

	/** Scale applied to the default backplate component to pad out the size. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|General", BlueprintGetter = "GetPlatedPadding",
		BlueprintSetter = "SetPlatedPadding")
	FVector PlatedPadding = FVector(1.0f, 1.0f, 1.0f);

	/** Number of columns or rows in respect to ViewableArea and ScrollDirection. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|Pagination", BlueprintGetter = "GetTiers", BlueprintSetter = "SetTiers",
		meta = (UIMin = "1"))
	int32 Tiers = 2;

	/** Number of lines visible in scroller. Orthogonal to tiers. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|Pagination", BlueprintGetter = "GetViewableArea",
		BlueprintSetter = "SetViewableArea", meta = (UIMin = "1"))
	int32 ViewableArea = 4;

	/** Depth, width, and height of cell per object. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection|Pagination", BlueprintGetter = "GetCellSize",
		BlueprintSetter = "SetCellSize")
	FVector CellSize = FVector(1.6f, 3.2f, 3.2f);

	/** Collision profile used by the box collider. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Scrolling Object Collection", AdvancedDisplay, BlueprintGetter = "GetCollisionProfile",
		BlueprintSetter = "SetCollisionProfile")
	FName CollisionProfile = TEXT("UI");

	/** Current Poke Target we are passing messages to. */
	UPROPERTY()
	TScriptInterface<IUxtPokeTarget> PokeTarget;

	/** Current Far Target we are passing messages to. */
	UPROPERTY()
	TScriptInterface<IUxtFarTarget> FarTarget;

	/** Cached calculated scrollable bounds of the collection. */
	FScrollingCollectionScrollableBounds ScrollableBounds;

	/** If not null, near pointer that is currently the focus of any interaction. */
	TWeakObjectPtr<UUxtNearPointerComponent> PokePointer;

	/** If not null, far pointer that is currently the focus of any interaction. */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointer;

	/** Local space offset along the direction of scroll at the start of the interaction. */
	float InteractionOrigin = 0.0f;

	/** Last local space offset along the direction of scroll, relative to #InteractionOrigin. */
	float InteractionOffset = 0.0f;

	/** The previous interaction offset. Used to update #InteractionVelocityOffset. */
	float PrevInteractionOffset = 0.0f;

	/** Offset in the direction of scroll, does not include any current interaction contribution. */
	float Offset = 0.0f;

	/** Current velocity of the offset. */
	float OffsetVelocity = 0.0f;

	/** If non zero there has been a pagination request. Pagination is treated as a modal operation that blocks all other interaction. */
	float PaginationDelta = 0.0f;

	/** The current offset resulting from any ongoing pagination. Always applied but non zero only if #PaginationDelta is non zero. */
	float PaginationOffset = 0.0f;

	/** The current time in the pagination. The pagination progress is determined by using this value to evaluate #PaginationCurve */
	float PaginationTime = 0.0f;

	/** Pagination complete delegate. */
	FUxtScrollingObjectCollectionOnPaginationEnd OnPaginationComplete;

	/** Has hit the #ClickMovementThreshold. */
	bool bHasHitClickMovementThreshold = false;

	/** Time we wait for a scroll before passing the click action onto the button. */
	float ScrollOrClickTime = 0.15f;

	/** Handle for the callback to check if we are scrolling or clicking. */
	FTimerHandle ScrollOrClickHandle;

#if WITH_EDITORONLY_DATA
	/** Record whether the collection has been initialized at least once while in the editor. */
	bool bCollectionInitializedInEditor = false;
#endif // WITH_EDITORONLY_DATA
};
