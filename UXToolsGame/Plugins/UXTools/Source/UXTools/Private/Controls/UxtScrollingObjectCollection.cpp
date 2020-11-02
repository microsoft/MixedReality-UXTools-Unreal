// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtScrollingObjectCollection.h"

#include "DrawDebugHelpers.h"
#include "TimerManager.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Controls/UxtCollectionObject.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#define check_validscrolldirection()                                                                               \
	checkf(                                                                                                        \
		ScrollDirection == EUxtScrollDirection::UpAndDown || ScrollDirection == EUxtScrollDirection::LeftAndRight, \
		TEXT("Unsupported scroll direction."))

namespace
{
	template <EInteractionTypeBits T>
	constexpr int32 InteractionTypeToBit()
	{
		return 1 << static_cast<int32>(T);
	}

	template <EInteractionTypeBits T>
	constexpr inline bool TestInteractionTypeBitmask(const int32 Bitmask)
	{
		return Bitmask & InteractionTypeToBit<T>();
	}
} // namespace

const int32 ScrollingObjectCollectionMinTiers = 1;

/**
 *
 */
UUxtScrollingObjectCollection::UUxtScrollingObjectCollection()
	: CanScroll(
		  InteractionTypeToBit<EInteractionTypeBits::NearInteraction>() | InteractionTypeToBit<EInteractionTypeBits::FarInteraction>() |
		  InteractionTypeToBit<EInteractionTypeBits::TouchInteraction>())
	, ScrollDirection(EUxtScrollDirection::UpAndDown)
	, CellWidth(3.2f)
	, CellHeight(3.2f)
	, ViewableArea(4)
	, CollisionProfile(TEXT("UI"))
	, bReleaseAtScrollBoundary(false)
	, ScrollSmoothing(0.5f)
	, VelocityDamping(0.04f)
	, BounceSpringFactor(0.75f)
	, SnapToStrength(0.0f)
	, Tiers(2)
	, BoxComponent(nullptr)
	, CollectionRoot(nullptr)
	, Offset(0.0f)
	, OffsetVelocity(0.0f)
	, PaginationDelta(0.0f)
	, PaginationOffset(0.0f)
	, PaginationTime(0.0f)
#if WITH_EDITORONLY_DATA
	, bCollectionInitializedInEditor(false)
#endif // WITH_EDITORONLY_DATA
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;

	// ...
#if WITH_EDITORONLY_DATA
	bTickInEditor = true;
#endif // WITH_EDITORONLY_DATA

	// By default the PaginationCurve will contain no keys, that isn't very user friendly and can result in an invalid curve
	// if not attended to by the user. Instead we should supply a sensible default curve.
	FRichCurve* const Curve = PaginationCurve.GetRichCurve();
	if (Curve)
	{
		const bool bAutoSetTangents = true;
		Curve->SetKeyInterpMode(Curve->AddKey(0.0f, 0.0f), ERichCurveInterpMode::RCIM_Cubic, bAutoSetTangents);
		Curve->SetKeyInterpMode(Curve->AddKey(0.5f, 1.0f), ERichCurveInterpMode::RCIM_Cubic, bAutoSetTangents);
	}

	ScrollOrClickTime = 0.25f;
	ClickMovementThreshold = 2.0f;
}

/**
 *
 */
EUxtScrollDirection UUxtScrollingObjectCollection::GetScrollDirection()
{
	return ScrollDirection;
}

/**
 *
 */
void UUxtScrollingObjectCollection::BeginPlay()
{
	Super::BeginPlay();

	// Create box component to help identify collision events
	BoxComponent = NewObject<UBoxComponent>(this, "Scrolling List Box");
	check(BoxComponent);
	BoxComponent->SetupAttachment(this);
	BoxComponent->RegisterComponent();

	// Need to ensure that the collection is initialized at run time. We store a array of child actors that is not serializable,
	// and it is not possible to guarantee that we can recreate this array in the same order as when the collection was last initialized
	// if no sorting predicate is supplied.
	InitializeCollection();

	// We also need to ensure that the box component is correct the collection has been initialized
	// It might seem sensible to call ConfigureBoxComponent from within InitializeCollection, however
	// InitializeCollection will be called while in editor, any time that a property is edited, which
	// is before the box component is created here in BeginPlay. The collection properties are constant
	// while in play so we should never need to reconfigure the box component after this point.
	// Unless components are added at runtime, in which case we are after BeginPlay has been called anyway
	ConfigureBoxComponent();

	// Create a new scene component to act as a root attachment point for the attached actors ...
	CollectionRoot = NewObject<USceneComponent>(this);
	check(CollectionRoot);
	CollectionRoot->SetupAttachment(this);
	CollectionRoot->RegisterComponent();
	// ... and then make sure to attach the actors.
	for (AActor* const Actor : GetAttachedActors())
	{
		Actor->AttachToComponent(CollectionRoot, FAttachmentTransformRules::KeepWorldTransform);
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
#if WITH_EDITORONLY_DATA
	// There doesn't seem to be a virtual function that is called to initialize the component only in editor and when
	// child actors have themselves been initialized and are available. Hence, in the constructor we have allowed
	// tick in editor, and here we will initialize the collection once before disallowing any further tick in editor.
	// Note that this is only done to ensure that the collection is presented correctly in the editor, before any properties
	// are edited. At runtime the collection is initialized in BeginPlay (as InitializeComponent seems not to be called in some
	// circumstances!)
	if (!bCollectionInitializedInEditor)
	{
		InitializeCollection();
		bTickInEditor = false;
		return;
	}
#endif // WITH_EDITORONLY_DATA

	TickCollectionOffset(DeltaTime);
	ResetCollectionVisibility();
}

void UUxtScrollingObjectCollection::DestroyComponent(bool bPromoteToChildred)
{
	if (ScrollOrClickHandle.IsValid())
	{
		ScrollOrClickHandle.Invalidate();
	}
	Super::DestroyComponent(bPromoteToChildred);
}

/**
 *
 */
void UUxtScrollingObjectCollection::TickCollectionOffset(const float DeltaTime)
{
	if (PaginationDelta != 0.0f)
	{
		// If we have a non zero pagination delta at the same time as an active interaction it means that either PageBy or MoveByItems was
		// called and the option to allow while interactions was true, or the pagination request go there just before the interaction
		// started. In either case it seems reasonable for the pagination to take priority over the active interaction.
		float Progress;
		if (VerifyAndEvaluatePaginationCurve(PaginationTime += DeltaTime, &Progress))
		{
			// Tempting to clamp progress in the range [0, 1], but let's allow the user to be creative if that is what they want.
			PaginationOffset = Progress * PaginationDelta;
		}
		else
		{
			// pagination is done, clean up in a similar was to the interaction.
			// Add our final pagination delta to the current offset, and then zero it out so we don't come back here next tick
			Offset += PaginationDelta;
			PaginationDelta = 0.0f;
			// Zero the pagination offset, Offset now accounts for this value and we don't want to add it again below.
			// Note that if the final key of the curve does not evaluate to 1.0f we may see a jump here. But that may be what is desired.
			PaginationOffset = 0.0f;
			// Finally call the callback
			OnPaginationComplete.ExecuteIfBound(EUxtPaginateResult::Success);
		}
	}
	else if (IsActiveInteraction())
	{
		// Recalculate the interaction velocity based on last and current interaction offset
		// this is inherently noisy so we should smooth the value
		OffsetVelocity = FMath::Lerp(OffsetVelocity, (InteractionOffset - PrevInteractionOffset) / DeltaTime, 0.1f);
		// update the previous value ready for next tick
		PrevInteractionOffset = InteractionOffset;
	}
	else
	{
		float MinimumValidOffset, MaximumValidOffset;
		GetValidOffsetRange(&MinimumValidOffset, &MaximumValidOffset);

		if (Offset < MinimumValidOffset)
		{
			const float ReboundVelocity = (MinimumValidOffset - Offset) * BounceSpringFactor;
			OffsetVelocity += ReboundVelocity;

			const float NextOffset = Offset + OffsetVelocity * DeltaTime;
			if (OffsetVelocity > 0.0f && NextOffset >= MinimumValidOffset)
			{
				OffsetVelocity = 0.0f;
				Offset = MinimumValidOffset;
			}
			else
			{
				Offset = NextOffset;
			}
		}
		else if (Offset > MaximumValidOffset)
		{
			const float ReboundVelocity = (MaximumValidOffset - Offset) * BounceSpringFactor;
			OffsetVelocity += ReboundVelocity;

			const float NextOffset = Offset + OffsetVelocity * DeltaTime;
			if (OffsetVelocity < 0.0f && NextOffset <= MaximumValidOffset)
			{
				OffsetVelocity = 0.0f;
				Offset = MaximumValidOffset;
			}
			else
			{
				Offset = NextOffset;
			}
		}

		// Is the Offset beyond valid extents, if so we want to be able to spring back
		if (Offset < MinimumValidOffset)
		{
			const float ReboundVelocity = (MinimumValidOffset - Offset) * BounceSpringFactor;
			OffsetVelocity += ReboundVelocity;

			const float NextOffset = Offset + OffsetVelocity * DeltaTime;
			if (OffsetVelocity > 0.0f && NextOffset >= MinimumValidOffset)
			{
				OffsetVelocity = 0.0f;
				Offset = MinimumValidOffset;
			}
			else
			{
				Offset = NextOffset;
			}
		}
		else if (Offset > MaximumValidOffset)
		{
			const float ReboundVelocity = (MaximumValidOffset - Offset) * BounceSpringFactor;
			OffsetVelocity += ReboundVelocity;

			const float NextOffset = Offset + OffsetVelocity * DeltaTime;
			if (OffsetVelocity < 0.0f && NextOffset <= MaximumValidOffset)
			{
				OffsetVelocity = 0.0f;
				Offset = MaximumValidOffset;
			}
			else
			{
				Offset = NextOffset;
			}
		}
		else
		{
			// Offset to the nearest snap to location, i.e. the nearest cell boundary
			const float SingleCellOffset = GetSingleCellOffset();
			const float OffsetDeltaToSnap = (SingleCellOffset * FMath::RoundToFloat(Offset / SingleCellOffset)) - Offset;

			// We will add to the offset and scale the velocity based on the strength of the snap to effect
			// The result should be that we stick to the snap location if we don't already have enough velocity to take us past
			// it and towards the next location. Note that a strength of 1.0f will result in an instant snap to the location and a zeroing
			// of the velocity
			Offset += OffsetDeltaToSnap * SnapToStrength;
			OffsetVelocity = FMath::Lerp(OffsetVelocity, 0.0f, SnapToStrength);

			// Update the offset with any residual velocity
			Offset += OffsetVelocity * DeltaTime;

			// Dampen the velocity. Zero the velocity below a threshold to prevent potentially ugly slow crawl when the numbers get very
			// small.
			if (FMath::Abs(OffsetVelocity = FMath::Lerp(OffsetVelocity, 0.0f, VelocityDamping)) < 0.0001f)
			{
				OffsetVelocity = 0.0f;
			}
		}
	}

	// apply the calculated offset based on the InteractionOffset, this should be valid to do regardless
	// of whether there is an active interaction or not.
	CollectionRoot->SetRelativeLocation(
		GetCurrentNetOffset() * (ScrollDirection == EUxtScrollDirection::UpAndDown ? FVector::UpVector : FVector::RightVector));
}

/**
 *
 */
void UUxtScrollingObjectCollection::InitializeCollection()
{
	Tiers = FMath::Max(Tiers, ScrollingObjectCollectionMinTiers); // Make sure no one sets 0;
	const TArray<AActor*>& Actors = CollectAttachedActors();

	// spin through attached actors and place them
	// we fill actors in order, filling up 'tiers' first
	const float TierDir = -1.0f;
	const float OrthoDir = -1.0f;

	// If the @ScrollDirection property is set to LeftAndRight then all that needs to be done at the point
	// is to swap the axis in which the offsets are applied, placement logic remains the same
	// Use pointers to member to make this transparent within the loop below.
	float FVector::*pTier = &FVector::Y;
	float FVector::*pOrtho = &FVector::Z;
	float TierOffset = TierDir * CellWidth;
	float OrthoOffset = OrthoDir * CellHeight;
	if (ScrollDirection == EUxtScrollDirection::LeftAndRight)
	{
		pTier = &FVector::Z;
		pOrtho = &FVector::Y;
		TierOffset = TierDir * CellHeight;
		OrthoOffset = OrthoDir * CellWidth;
	}

	// The logic above avoids warnings for uninitialized local variables but implicitly assumes only two possible values.
	// Should anyone add a third scroll direction in the future this will need to be changed, and we should help that person find this code
	// Note: Rather than littering this class with asserts we will assert once here, this is not the only place this assumption is made.
	check_validscrolldirection();

	FVector ActorLocation = FVector::ZeroVector;

	// Outer loop iterates through actor array, one tier at a time
	for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex += Tiers)
	{
		// Inner loop iterates through actors that should be placed within the tier
		// but we need to account for the fact that we might not have enough actors to fill the tier
		const int32 NumColumnsInRow = FMath::Min(Tiers, Actors.Num() - ActorIndex);
		for (int32 ColumnIndex = 0; ColumnIndex < NumColumnsInRow; ++ColumnIndex)
		{
			Actors[ActorIndex + ColumnIndex]->SetActorRelativeLocation(ActorLocation);

			// increment location offset in tier direction ready for the next actor
			ActorLocation.*pTier += TierOffset;
		}
		// reset location offset in tier direction, and increment tier orthogonal offset ready for the start of the next tier
		ActorLocation.*pTier = 0.0f;
		ActorLocation.*pOrtho += OrthoOffset;
	}

	// Collection properties may have changed to we need to update the visibility of those contained actors
	ResetCollectionVisibility();

	// SetupCollection Properties for broadcast
	FScrollingCollectionProperties Properties;

	FVector Size = FVector::ZeroVector;
	Size.*pTier = TierDir * TierOffset * Tiers;
	Size.*pOrtho = OrthoDir * OrthoOffset * ViewableArea;
	Properties.Width = Size.Z;
	Properties.Height = Size.Y;

	Properties.Center = FVector::ZeroVector;
	Properties.Center.*pTier = TierOffset * (Tiers - 1) * 0.5f;
	Properties.Center.*pOrtho = OrthoOffset * (ViewableArea - 1) * 0.5f;

	if (BackPlate)
	{
		FVector CurrentRelative = BackPlate->GetRelativeLocation();
		CurrentRelative.Y = Properties.Center.Y;
		CurrentRelative.Z = Properties.Center.Z;
		BackPlate->SetRelativeLocation(CurrentRelative);
		BackPlate->SetRelativeScale3D(FVector(UUxtBackPlateComponent::GetDefaultBackPlateDepth(), Properties.Height, Properties.Width));
	}

	// Raise event so that the containing Blueprint can respond
	if (OnCollectionUpdated.IsBound())
	{
		OnCollectionUpdated.Broadcast(Properties);
	}

#if WITH_EDITORONLY_DATA
	bCollectionInitializedInEditor = true;
#endif // WITH_EDITORONLY_DATA
}

/**
 *
 */
void UUxtScrollingObjectCollection::ResetCollectionVisibility()
{
	// Based on the current total offset of the collection we can determine which items are visible
	// within the viewable area. The offset is signed such that an offset below zero mean that the
	// first visible entry would have a negative index (if it existed), conversely a positive offset
	// means that entry 0 is off the front of the viewable area and we can see a section of entries
	// starting with  a later entry.
	// When calculating the first visible we should round down; similarly we should round up for the
	// last visible. Hence we can't rely on an implicit float to in cast.
	const float CellDimension = GetSingleCellOffset();
	const int FirstVisibleRow = FMath::FloorToInt(GetCurrentNetOffset() / CellDimension);
	const int FirstNotVisibleRow = FMath::CeilToInt((GetCurrentNetOffset() / CellDimension) + ViewableArea);

	int FirstVisible = FMath::Max(FirstVisibleRow, 0) * Tiers;
	int FirstNotVisible = FMath::Max(FirstNotVisibleRow, 0) * Tiers;

	const TArray<AActor*>& Actors = GetAttachedActors();
	for (int i = 0; i < Actors.Num(); ++i)
	{
		const bool bHidden = (i < FirstVisible || i >= FirstNotVisible);
		Actors[i]->SetActorHiddenInGame(bHidden);
		Actors[i]->SetActorTickEnabled(!bHidden);
		Actors[i]->SetActorEnableCollision(!bHidden);

#if WITH_EDITORONLY_DATA
		Actors[i]->SetIsTemporarilyHiddenInEditor(bHidden);
#endif // WITH_EDITORONLY_DATA
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::ConfigureBoxComponent()
{
	checkf(BoxComponent, TEXT("Attempting to configure box component before it has been created in BeginPlay."));

#if WITH_EDITORONLY_DATA
	check(bCollectionInitializedInEditor);
#endif // WITH_EDITORONLY_DATA

	// Disable collision on all of the parent actor's primitive components so that we can use the box component to identify collisions
	// note that this will not affect any of the collection's actors as they are attached in the scene hierarchy.
	// Also note that this will disable the BoxComponent's collision, which will then be explicitly enabled below.
	const AActor* const Owner = GetOwner();
	if (Owner)
	{
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
		const bool bIncludeFromChildActors = false;
		Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents, bIncludeFromChildActors);
		for (UPrimitiveComponent* const Component : PrimitiveComponents)
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		// Expand a bounding box to encapsulate all of the attached actors that are currently
		// within the viewable area (we can assume that all actors with collision enabled are included)
		// we are operating in component local space
		FBox BoundingBox(EForceInit::ForceInit);
		const FTransform& WorldToLocal = GetComponentTransform().Inverse();
		for (AActor* const Actor : GetAttachedActors())
		{
			if (Actor->GetActorEnableCollision())
			{
				const bool bNonColliding = false;
				BoundingBox += UUxtMathUtilsFunctionLibrary::CalculateNestedActorBoundsInGivenSpace(Actor, WorldToLocal, bNonColliding);
			}
		}

		// Expand the front face of the bounds by a small percentage
		const float Min = BoundingBox.Min.X;
		const float Max = BoundingBox.Max.X;
		BoundingBox.Max.X = FMath::Lerp(Min, Max, 1.1f);

		BoxComponent->SetWorldTransform(FTransform(BoundingBox.GetCenter()) * GetComponentTransform());
		BoxComponent->SetBoxExtent(BoundingBox.GetExtent());
		BoxComponent->SetCollisionProfileName(CollisionProfile);

		// Make sure to re enable collision on the box component
		BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// Cache the extents for use in a workaround in OnBeginPoke_Implementation and OnEndPoke_Implementation
		BoxComponentExtents = BoundingBox.GetExtent();
	}
}

/**
 *
 */
float UUxtScrollingObjectCollection::GetScrollOffsetFromLocation(const FVector LocalSpaceLocation) const
{
	// We know that the scroll offset is aligned to one of two possible axes in local space.
	const float FVector::*pOffset = (ScrollDirection == EUxtScrollDirection::UpAndDown ? &FVector::Z : &FVector::Y);
	return LocalSpaceLocation.*pOffset;
}

float UUxtScrollingObjectCollection::GetTierOffsetFromLocation(const FVector LocalSpaceLocation) const
{
	// We know that the tier offset is aligned to one of two possible axes in local space.
	const float FVector::*pOffset = (ScrollDirection == EUxtScrollDirection::UpAndDown ? &FVector::Y : &FVector::Z);
	return LocalSpaceLocation.*pOffset;
}

/**
 *
 */
int UUxtScrollingObjectCollection::GetNumberOfRowsInCollection() const
{
	const int NoofActors = GetAttachedActors().Num();
	int NoofRows = (NoofActors / Tiers);
	// A remainder means that we need an extra row.
	if (NoofActors % Tiers > 0)
	{
		NoofRows++;
	}
	return NoofRows;
}

/**
 *
 */
float UUxtScrollingObjectCollection::GetSingleCellOffset() const
{
	return (ScrollDirection == EUxtScrollDirection::UpAndDown ? CellHeight : CellWidth);
}

/**
 *
 */
bool UUxtScrollingObjectCollection::IsActiveInteraction() const
{
	return PokePointer.IsValid() || FarPointer.IsValid();
}

void UUxtScrollingObjectCollection::SetTiers(int32 IncomingTiers)
{
	Tiers = FMath::Max(ScrollingObjectCollectionMinTiers, IncomingTiers);
}

/**
 *
 */
void UUxtScrollingObjectCollection::PageBy(
	const int32 NumPages, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& callback)
{
	MoveByItems(NumPages * ViewableArea, bAnimate, callback);
}

/**
 *
 */
void UUxtScrollingObjectCollection::MoveByItems(
	const int32 NumItems, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& Callback)
{
	// In order to keep things simple lets not allow the pagination request if ...
	// ... the collection is already in the middle of another pagination.
	if (PaginationDelta != 0.0f)
	{
		// call callback with failure reason
		Callback.ExecuteIfBound(EUxtPaginateResult::Failed_ConcurrentOperation);
		return;
	}

	// ... the collection is currently being interacted with by the user.
	if (IsActiveInteraction())
	{
		// call callback with failure reason
		Callback.ExecuteIfBound(EUxtPaginateResult::Failed_ConcurrentInteraction);
		return;
	}

	// What is the target offset that will take us where we want to be?
	// Remember to account for the possibility that we are halfway into a cell
	// Also we should clamp the current net offset to the allowable range to adjust for the
	// edge case where we are mid bounce and don't want that to effect where we end up
	float MinOffset, MaxOffset;
	GetValidOffsetRange(&MinOffset, &MaxOffset);
	const float EffectiveOffset = FMath::Clamp(GetCurrentNetOffset(), MinOffset, MaxOffset);
	const float SingleCellOffset = GetSingleCellOffset();
	const float CurrentOffsetIntoCell =
		SingleCellOffset * (EffectiveOffset / SingleCellOffset - FMath::FloorToFloat(EffectiveOffset / SingleCellOffset));

	// what is the valid range of offset deltas that can be applied by this pagination
	// i.e. we don't want the pagination to scroll past the end of the collection
	float PaginationDestination = EffectiveOffset + NumItems * GetSingleCellOffset() - CurrentOffsetIntoCell;
	PaginationDelta = FMath::Clamp(PaginationDestination, MinOffset, MaxOffset) - GetCurrentNetOffset();

	if (PaginationDelta == 0.0f)
	{
		// The paginate has been clamped to zero and will not finish because it will effectively never start
		Callback.ExecuteIfBound(EUxtPaginateResult::Success);
		return;
	}

	// We are going to handle not animating by just setting the time to be that of the last key.
	// When we next update the curve will be evaluated at that time and the pagination will end immediately.
	// Saves yet another logical branch to handle just this situation.
	PaginationTime = bAnimate ? 0.0f : PaginationCurve.GetRichCurve()->GetLastKey().Time;

	OnPaginationComplete = Callback;
}

void UUxtScrollingObjectCollection::AddActorToCollection(AActor* ActorToAdd)
{
	if (ActorToAdd)
	{
		ActorToAdd->AttachToComponent(CollectionRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ActorToAdd->SetActorRelativeTransform(FTransform::Identity);
		InitializeCollection();
		ConfigureBoxComponent();
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::GetValidOffsetRange(float* const Minimum, float* const Maximum) const
{
	*Minimum = 0.0f;
	*Maximum = (GetNumberOfRowsInCollection() - ViewableArea) * GetSingleCellOffset();
}

int UUxtScrollingObjectCollection::ConvertWorldSpacePositionToButtonIndex(FVector WorldSpaceLocation)
{
	// Setup like we did for the initial layout
	const float TierDir = -1.0f;
	const float OrthoDir = -1.0f;

	float TierOffset = CellWidth;
	float OrthoOffset = CellHeight;
	float TierBoundsOffset = 0.5f;
	float OrthoBoundsOffset = -0.5f;

	if (ScrollDirection == EUxtScrollDirection::LeftAndRight)
	{
		TierOffset = CellHeight;
		OrthoOffset = CellWidth;
		TierBoundsOffset = -0.5f;
		OrthoBoundsOffset = 0.5f;
	}

	FVector LocalSpacePosition = CollectionRoot->GetComponentTransform().InverseTransformPosition(WorldSpaceLocation);

	// Map local space position onto grid

	// Get Row
	float OrthPosition = GetScrollOffsetFromLocation(LocalSpacePosition) * OrthoDir;
	OrthPosition += OrthoBoundsOffset * OrthoOffset; // offset by 1/2 height to account for bounds : middle of actor isn't the boundary
	int SelectedRow = FMath::Max(0, FMath::FloorToInt(OrthPosition / OrthoOffset));
	SelectedRow = SelectedRow * Tiers;
	// Get Column
	float TierPosition = GetTierOffsetFromLocation(LocalSpacePosition) * TierDir;
	TierPosition += TierBoundsOffset * TierOffset; // offset by 1/2 height to account for bounds : middle of actor isn't the boundary
	int SelectedTier = FMath::Max(FMath::FloorToInt(TierPosition / TierOffset), 0);

	// Merge the column and row indexes
	return SelectedRow + SelectedTier;
}

void UUxtScrollingObjectCollection::CheckScrollOrClick()
{
	UUxtNearPointerComponent* Pointer = PokePointer.Get();
	if (Pointer && !bHasHitClickMovementThreshold)
	{
		FVector LocalSpaceLocation = LocationWorldToLocal(Pointer->GetGrabPointerTransform().GetLocation());
		float DeltaMove = FMath::Abs(GetScrollOffsetFromLocation(LocalSpaceLocation) - InteractionOrigin);

		if (DeltaMove < ClickMovementThreshold)
		{
			int ButtonIndex = ConvertWorldSpacePositionToButtonIndex(Pointer->GetGrabPointerTransform().GetLocation());

			if (ButtonIndex != -1)
			{
				const TArray<AActor*>& Actors = GetAttachedActors();
				if (Actors.IsValidIndex(ButtonIndex) && Actors[ButtonIndex])
				{
					if (Actors[ButtonIndex]->GetClass()->ImplementsInterface(UUxtCollectionObject::StaticClass()))
					{
						PokeTarget = IUxtCollectionObject::Execute_GetPokeTarget(Actors[ButtonIndex]);
						if (PokeTarget)
						{
							IUxtPokeHandler::Execute_OnBeginPoke(PokeTarget.GetObject(), Pointer);
						}
					}
				}
			}
		}
	}
}

void UUxtScrollingObjectCollection::CheckScrollOrClickFarPointer()
{
	UUxtFarPointerComponent* Pointer = FarPointer.Get();
	if (Pointer && !bHasHitClickMovementThreshold)
	{
		FVector LocalSpaceLocation = LocationWorldToLocal(Pointer->GetPointerOrigin());
		float DeltaMove = FMath::Abs(GetScrollOffsetFromLocation(LocalSpaceLocation) - InteractionOrigin);

		if (DeltaMove < ClickMovementThreshold)
		{
			int ButtonIndex = ConvertWorldSpacePositionToButtonIndex(Pointer->GetCursorTransform().GetLocation());

			if (ButtonIndex != -1)
			{
				const TArray<AActor*>& Actors = GetAttachedActors();
				if (Actors.IsValidIndex(ButtonIndex) && Actors[ButtonIndex])
				{
					if (Actors[ButtonIndex]->GetClass()->ImplementsInterface(UUxtCollectionObject::StaticClass()))
					{
						FarTarget = IUxtCollectionObject::Execute_GetFarTarget(Actors[ButtonIndex]);
						if (FarTarget)
						{
							IUxtFarHandler::Execute_OnFarPressed(FarTarget.GetObject(), Pointer);
						}
					}
				}
			}
		}
	}
}

/**
 *
 */
bool UUxtScrollingObjectCollection::VerifyAndEvaluatePaginationCurve(const float EvalTime, float* const Output) const
{
	float MinTime, MaxTime;
	PaginationCurve.GetRichCurveConst()->GetTimeRange(MinTime, MaxTime);
	if (EvalTime >= MinTime && EvalTime <= MaxTime)
	{
		*Output = PaginationCurve.GetRichCurveConst()->Eval(EvalTime);
		return true;
	}
	else
	{
		return false;
	}
}

#if WITH_EDITORONLY_DATA
/**
 *
 */
void UUxtScrollingObjectCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	InitializeCollection();
}

#endif // WITH_EDITORONLY_DATA

bool UUxtScrollingObjectCollection::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

bool UUxtScrollingObjectCollection::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

void UUxtScrollingObjectCollection::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (!TestInteractionTypeBitmask<EInteractionTypeBits::NearInteraction>(CanScroll))
	{
		return;
	}

	check(Pointer);

	if (!IsActiveInteraction())
	{
		PokePointer = Pointer;
		// Need to lock focus so that nothing contained within the collection has the opportunity to do so
		// otherwise things start to get quite messy. Events will be passed to objects contained by the
		// collection if they implement the interface IUxtCollectionObject.
		Pointer->SetFocusLocked(true);

		FVector LocalSpaceLocation = LocationWorldToLocal(Pointer->GetGrabPointerTransform().GetLocation());
		InteractionOrigin = GetScrollOffsetFromLocation(LocalSpaceLocation);
		InteractionOffset = 0.0f;
		PrevInteractionOffset = 0.0f;
		OffsetVelocity = 0.0f;

		bHasHitClickMovementThreshold = false;
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(
				ScrollOrClickHandle, this, &UUxtScrollingObjectCollection::CheckScrollOrClick, ScrollOrClickTime, false);
		}

		// Work around for issue when bReleaseAtScrollBoundary is false. In this case EUxtPokeBehaviour::FrontFace
		// is returned from GetPokeBehaviour_Implementation. This results in a call to IsFrontFacePokeEnded (UxtNearPointerComponent.cpp)
		// from UUxtNearPointerComponent::UpdatePokeInteraction. From a casual reading of that function it appears as if the
		// SphereAABBIntersection test will result in the test failing if the cursor moves out of the side of the box, and not just if the
		// cursor is in front of the front face. Again, this is a very casual reading of the function but it does appear to match the
		// behaviour the we are seeing. So to work around this temporarily, we can just pad out the extents of the box in Y and Z directions.
		if (!bReleaseAtScrollBoundary)
		{
			BoxComponent->SetBoxExtent(FVector(BoxComponentExtents.X, BoxComponentExtents.Y * 2.0f, BoxComponentExtents.Z * 2.0f));
		}
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (PokePointer.Get() == Pointer)
	{
		InteractionOffset = FMath::Lerp(
			InteractionOffset,
			GetScrollOffsetFromLocation(LocationWorldToLocal(Pointer->GetGrabPointerTransform().GetLocation())) - InteractionOrigin,
			ScrollSmoothing);

		// If, at any point, InteractionOffset becomes larger that the ClickMovementThreshold then we need to record that fact, and alert
		// any interested parties.
		if (FMath::Abs(InteractionOffset) > ClickMovementThreshold)
		{
			bHasHitClickMovementThreshold = true;
		}
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (PokePointer.Get() == Pointer)
	{
		Pointer->SetFocusLocked(false);
		PokePointer = nullptr;
		Offset += InteractionOffset;
		InteractionOffset = 0.0f;

		if (ScrollOrClickHandle.IsValid())
		{
			ScrollOrClickHandle.Invalidate();
		}

		if (PokeTarget)
		{
			IUxtPokeHandler::Execute_OnEndPoke(PokeTarget.GetObject(), Pointer);
			PokeTarget = nullptr;
		}

		// Workaround for bReleaseAtScrollBoundary issue. See OnBeginPoke_Implementation for explanation.
		// Always reset the box extents just in case bReleaseAtScrollBoundary was changed in between begin poke and now
		BoxComponent->SetBoxExtent(BoxComponentExtents);
	}
}

EUxtPokeBehaviour UUxtScrollingObjectCollection::GetPokeBehaviour_Implementation() const
{
	// Note: See workaround in OnBeginPoke_Implementation and OnEndPoke_Implementation for case where bReleaseAtScrollBoundary == false
	return bReleaseAtScrollBoundary ? EUxtPokeBehaviour::Volume : EUxtPokeBehaviour::FrontFace;
}

bool UUxtScrollingObjectCollection::GetClosestPoint_Implementation(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	OutNormal = GetComponentTransform().GetUnitAxis(EAxis::X);

	float NotUsed;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed);
}

bool UUxtScrollingObjectCollection::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

bool UUxtScrollingObjectCollection::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

void UUxtScrollingObjectCollection::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (!TestInteractionTypeBitmask<EInteractionTypeBits::FarInteraction>(CanScroll))
	{
		return;
	}

	check(Pointer);

	if (!IsActiveInteraction())
	{
		FarPointer = Pointer;
		// Need to lock focus so that nothing contained within the collection has the opportunity to do so
		// otherwise things start to get quite messy. Events will be passed to objects contained by the
		// collection if they implement the interface IUxtCollectionObject.
		Pointer->SetFocusLocked(true);

		FVector LocalSpaceLocation = LocationWorldToLocal(Pointer->GetPointerOrigin());
		InteractionOrigin = GetScrollOffsetFromLocation(LocalSpaceLocation);
		InteractionOffset = 0.0f;
		PrevInteractionOffset = 0.0f;
		OffsetVelocity = 0.0f;

		bHasHitClickMovementThreshold = false;
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(
				ScrollOrClickHandle, this, &UUxtScrollingObjectCollection::CheckScrollOrClickFarPointer, ScrollOrClickTime, false);
		}
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (FarPointer.Get() == Pointer)
	{
		InteractionOffset = FMath::Lerp(
			InteractionOffset, GetScrollOffsetFromLocation(LocationWorldToLocal(Pointer->GetPointerOrigin())) - InteractionOrigin,
			ScrollSmoothing);

		// If, at any point, InteractionOffset becomes larger that the ClickMovementThreshold then we need to record that fact, and alert
		// any interested parties.
		if (FMath::Abs(InteractionOffset) > ClickMovementThreshold)
		{
			bHasHitClickMovementThreshold = true;
		}
	}
}

/**
 *
 */
void UUxtScrollingObjectCollection::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (FarPointer.Get() == Pointer)
	{
		Pointer->SetFocusLocked(false);
		FarPointer = nullptr;
		Offset += InteractionOffset;
		InteractionOffset = 0.0f;

		if (ScrollOrClickHandle.IsValid())
		{
			ScrollOrClickHandle.Invalidate();
		}

		if (FarTarget)
		{
			IUxtFarHandler::Execute_OnFarReleased(FarTarget.GetObject(), Pointer);
			FarTarget = nullptr;
		}
	}
}
