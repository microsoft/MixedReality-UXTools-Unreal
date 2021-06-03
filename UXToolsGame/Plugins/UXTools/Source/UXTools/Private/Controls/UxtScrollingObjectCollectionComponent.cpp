// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtScrollingObjectCollectionComponent.h"

#include "DrawDebugHelpers.h"
#include "TimerManager.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Controls/UxtBackPlateComponent.h"
#include "Controls/UxtCollectionObject.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtInteractionUtils.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#include <Materials/MaterialInstanceDynamic.h>

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

const int32 ScrollingObjectCollectionMinDim = 1;

template <typename T>
void FindOrCreateSubComponent(T*& Component, UUxtScrollingObjectCollectionComponent* Outer, bool FindExisting = true)
{
	if (Component == nullptr)
	{
		// Look for the component as an existing attachment.
		if (FindExisting)
		{
			TArray<USceneComponent*> Children;
			Outer->GetChildrenComponents(false, Children);

			for (USceneComponent* Child : Children)
			{
				if (!Child->IsBeingDestroyed())
				{
					if (T* CastChild = Cast<T>(Child))
					{
						Component = CastChild;
						break;
					}
				}
			}
		}

		// If none was found, make one.
		if (Component == nullptr)
		{
			Component = NewObject<T>(Outer);
			Component->SetupAttachment(Outer);

			if (Outer->GetOwner() != nullptr)
			{
				Component->RegisterComponent();
			}
		}
	}
}

UUxtScrollingObjectCollectionComponent::UUxtScrollingObjectCollectionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
	bWantsOnUpdateTransform = true;

#if WITH_EDITORONLY_DATA
	bTickInEditor = true;
#endif // WITH_EDITORONLY_DATA

	// By default the PaginationCurve will contain no keys, that isn't very user friendly and can result in an invalid curve
	// if not attended to by the user. Instead we should supply a sensible default curve.
	FRichCurve* const Curve = PaginationCurve.GetRichCurve();
	if (Curve != nullptr)
	{
		const bool bAutoSetTangents = true;
		Curve->SetKeyInterpMode(Curve->AddKey(0.0f, 0.0f), ERichCurveInterpMode::RCIM_Cubic, bAutoSetTangents);
		Curve->SetKeyInterpMode(Curve->AddKey(0.5f, 1.0f), ERichCurveInterpMode::RCIM_Cubic, bAutoSetTangents);
	}
}

void UUxtScrollingObjectCollectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Need to ensure that the collection is initialized at run time. We store a array of child actors that is not serializable,
	// and it is not possible to guarantee that we can recreate this array in the same order as when the collection was last initialized
	// if no sorting predicate is supplied.
	RefreshCollection();
}

void UUxtScrollingObjectCollectionComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITORONLY_DATA
	// There doesn't seem to be a virtual function that is called to initialize the component only in editor and when
	// child actors have themselves been initialized and are available. Hence, in the constructor we have allowed
	// tick in editor, and here we will initialize the collection once before disallowing any further tick in editor.
	// Note that this is only done to ensure that the collection is presented correctly in the editor, before any properties
	// are edited. At runtime the collection is initialized in BeginPlay (as InitializeComponent seems not to be called in some
	// circumstances!)
	if (!bCollectionInitializedInEditor)
	{
		RefreshCollection();
		bTickInEditor = false;
		return;
	}
#endif // WITH_EDITORONLY_DATA

	TickCollectionOffset(DeltaTime);
	ResetCollectionVisibility();
}

void UUxtScrollingObjectCollectionComponent::DestroyComponent(bool bPromoteToChildred)
{
	if (ScrollOrClickHandle.IsValid())
	{
		ScrollOrClickHandle.Invalidate();
	}

	if (BackPlateMeshComponent != nullptr)
	{
		BackPlateMeshComponent->DestroyComponent(bPromoteToChildred);
	}

	if (BoxComponent != nullptr)
	{
		BoxComponent->DestroyComponent(bPromoteToChildred);
	}

	if (CollectionRootComponent != nullptr)
	{
		CollectionRootComponent->DestroyComponent(bPromoteToChildred);
	}

	Super::DestroyComponent(bPromoteToChildred);
}

void UUxtScrollingObjectCollectionComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

	UpdateClippingBox();
}

void UUxtScrollingObjectCollectionComponent::TickCollectionOffset(const float DeltaTime)
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
			// Add our final pagination delta to the current offset, and then zero it out so we don't come back here next tick.
			Offset += PaginationDelta;
			PaginationDelta = 0.0f;
			// Zero the pagination offset, Offset now accounts for this value and we don't want to add it again below.
			// Note that if the final key of the curve does not evaluate to 1.0f we may see a jump here. But that may be what is desired.
			PaginationOffset = 0.0f;
			// Finally call the callback
			OnPaginationComplete.ExecuteIfBound(EUxtPaginateResult::Success);
		}
	}
	else if (IsActiveInteraction() && bHasHitClickMovementThreshold)
	{
		// Recalculate the interaction velocity based on last and current interaction offset
		// this is inherently noisy so we should smooth the value
		OffsetVelocity = FMath::Lerp(OffsetVelocity, (InteractionOffset - PrevInteractionOffset) / DeltaTime, 0.1f);
		// Update the previous value ready for next tick.
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

		// Is the Offset beyond valid extents, if so we want to be able to spring back.
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
			// Offset to the nearest snap to location, i.e. the nearest cell boundary.
			const float SingleCellOffset = GetSingleCellOffset();
			const float OffsetDeltaToSnap = (SingleCellOffset * FMath::RoundToFloat(Offset / SingleCellOffset)) - Offset;

			// We will add to the offset and scale the velocity based on the strength of the snap to effect.
			// The result should be that we stick to the snap location if we don't already have enough velocity to take us past
			// it and towards the next location. Note that a strength of 1.0f will result in an instant snap to the location and a zeroing
			// of the velocity
			Offset += OffsetDeltaToSnap * SnapToStrength;
			OffsetVelocity = FMath::Lerp(OffsetVelocity, 0.0f, SnapToStrength);

			// Update the offset with any residual velocity.
			Offset += OffsetVelocity * DeltaTime;

			// Dampen the velocity. Zero the velocity below a threshold to prevent potentially slow crawl when the numbers get very small.
			if (FMath::Abs(OffsetVelocity = FMath::Lerp(OffsetVelocity, 0.0f, VelocityDamping)) < 0.0001f)
			{
				OffsetVelocity = 0.0f;
			}
		}
	}

	// Apply the calculated offset based on the InteractionOffset, this should be valid to do regardless of whether there is an active
	// interaction or not.
	if (CollectionRootComponent != nullptr)
	{
		CollectionRootComponent->SetRelativeLocation(
			GetCurrentNetOffset() * (ScrollDirection == EUxtScrollDirection::UpAndDown ? FVector::UpVector : FVector::RightVector));
	}
}

void UUxtScrollingObjectCollectionComponent::RefreshCollection()
{
	Super::RefreshCollection();

	const TArray<AActor*>& Actors = CollectAttachedActors();

	// Ensure all actors are attached to the collection root when playing. We only want to do this when playing to make sure we don't
	// serialize the attachment of a instance component to a template component.
	if (HasBegunPlay())
	{
		// No need to find an existing component since this never exists in the editor.
		FindOrCreateSubComponent(CollectionRootComponent, this, false);

		for (AActor* const Actor : Actors)
		{
			if (Actor != nullptr)
			{
				Actor->AttachToComponent(CollectionRootComponent, FAttachmentTransformRules::KeepWorldTransform);
			}
		}
	}

	// Spin through attached actors and place them. We fill actors in order, filling up 'tiers' first.
	const float TierDir = -1.0f;
	const float OrthoDir = -1.0f;

	// If the @ScrollDirection property is set to LeftAndRight then all that needs to be done at the point is to swap the axis in which the
	// offsets are applied, placement logic remains the same. Use pointers to member to make this transparent within the loop below.
	float FVector::*pTier = &FVector::Y;
	float FVector::*pOrtho = &FVector::Z;
	float TierOffset = TierDir * CellSize.Y;
	float OrthoOffset = OrthoDir * CellSize.Z;
	if (ScrollDirection == EUxtScrollDirection::LeftAndRight)
	{
		pTier = &FVector::Z;
		pOrtho = &FVector::Y;
		TierOffset = TierDir * CellSize.Z;
		OrthoOffset = OrthoDir * CellSize.Y;
	}

	// The logic above avoids warnings for uninitialized local variables but implicitly assumes only two possible values.
	// Should anyone add a third scroll direction in the future this will need to be changed, and we should help that person find this code
	// Note: Rather than littering this class with asserts we will assert once here, this is not the only place this assumption is made.
	checkf(
		ScrollDirection == EUxtScrollDirection::UpAndDown || ScrollDirection == EUxtScrollDirection::LeftAndRight,
		TEXT("Unsupported scroll direction."));

	// Conditionally select the location/rotation based on how the actor is attached.
	const bool PlaceRelative = (GetOwner() && GetOwner()->GetRootComponent() == this) || HasBegunPlay();
	FVector ActorLocation = PlaceRelative ? FVector::ZeroVector : GetRelativeLocation();
	const FRotator ActorRotation = PlaceRelative ? FRotator::ZeroRotator : GetRelativeRotation();
	Tiers = FMath::Max(Tiers, ScrollingObjectCollectionMinDim); // Ensure we have at least 1 tier.

	// Outer loop iterates through actor array, one tier at a time
	for (int32 ActorIndex = 0; ActorIndex < Actors.Num(); ActorIndex += Tiers)
	{
		// Inner loop iterates through actors that should be placed within the tier
		// but we need to account for the fact that we might not have enough actors to fill the tier
		const int32 NumColumnsInRow = FMath::Min(Tiers, Actors.Num() - ActorIndex);
		for (int32 ColumnIndex = 0; ColumnIndex < NumColumnsInRow; ++ColumnIndex)
		{
			Actors[ActorIndex + ColumnIndex]->SetActorRelativeLocation(ActorLocation);
			Actors[ActorIndex + ColumnIndex]->SetActorRelativeRotation(ActorRotation);

			// Increment location offset in tier direction ready for the next actor.
			ActorLocation.*pTier += TierOffset;
		}
		// Reset location offset in tier direction, and increment tier orthogonal offset ready for the start of the next tier.
		ActorLocation.*pTier = PlaceRelative ? 0.0f : GetRelativeLocation().*pTier;
		ActorLocation.*pOrtho += OrthoOffset;
	}

	// Collection properties may have changed to we need to update the visibility of those contained actors
	ResetCollectionVisibility();

	// Expand the the depth of the bounds by 10% to ensure the bound encapsulate the contents.
	const float Expansion = 1.1f;

	// Update the ScrollableBounds for broadcast.
	ScrollableBounds.RelativeCenter = FVector::ForwardVector * (CellSize.X * Expansion);
	ScrollableBounds.RelativeCenter.*pTier = TierOffset * (Tiers - 1);
	ScrollableBounds.RelativeCenter.*pOrtho = OrthoOffset * (ViewableArea - 1);
	ScrollableBounds.RelativeCenter *= 0.5f;

	ScrollableBounds.Extents.X = CellSize.X * Expansion;
	ScrollableBounds.Extents.*pTier = TierDir * TierOffset * Tiers;
	ScrollableBounds.Extents.*pOrtho = OrthoDir * OrthoOffset * ViewableArea;
	ScrollableBounds.Extents *= 0.5f;

	ConfigureBackPlate();
	ConfigureBoxComponent();
	UpdateClippingBox();

	// Raise event so that the containing Blueprint can respond.
	if (OnCollectionUpdated.IsBound())
	{
		OnCollectionUpdated.Broadcast(ScrollableBounds);
	}

#if WITH_EDITORONLY_DATA
	bCollectionInitializedInEditor = true;
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITORONLY_DATA

void UUxtScrollingObjectCollectionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshCollection();
}
#endif // WITH_EDITORONLY_DATA

void UUxtScrollingObjectCollectionComponent::ResetCollectionVisibility()
{
	// Only update the visibility if the collection is visible.
	if (GetUIVisibilityInHierarchy() == EUxtUIElementVisibility::Show)
	{
		// Based on the current total offset of the collection we can determine which items are visible
		// within the viewable area. The offset is signed such that an offset below zero mean that the
		// first visible entry would have a negative index (if it existed), conversely a positive offset
		// means that entry 0 is off the front of the viewable area and we can see a section of entries
		// starting with a later entry.

		const float CellDimension = GetSingleCellOffset();
		const float Center = GetCurrentNetOffset() / CellDimension;

		// When calculating the first visible we should round down; similarly we should round up for the
		// last visible. But, when we are "very close" to being visible or invisible we should round the other way to avoid objects being
		// visible which are just touching the border.
		const float CenterMod = FMath::Fmod(FMath::Abs(Center), 1.0f);
		static const float BorderEpsilon = 0.001f;
		static const float OneMinusBorderEpsilon = 1.0f - BorderEpsilon;

		const int FirstVisibleRow = CenterMod > OneMinusBorderEpsilon ? FMath::CeilToInt(Center) : FMath::FloorToInt(Center);
		const int FirstNotVisibleRow =
			CenterMod < BorderEpsilon ? FMath::FloorToInt(Center) + ViewableArea : FMath::CeilToInt(Center) + ViewableArea;

		int FirstVisible = FMath::Max(FirstVisibleRow, 0) * Tiers;
		int FirstNotVisible = FMath::Max(FirstNotVisibleRow, 0) * Tiers;

		const TArray<AActor*>& Actors = GetAttachedActors();
		TArray<AActor*> ChildActors;
		for (int ActorIndex = 0; ActorIndex < Actors.Num(); ++ActorIndex)
		{
			AActor* Actor = Actors[ActorIndex];

			if (Actor != nullptr)
			{
				const bool bHidden = (ActorIndex < FirstVisible || ActorIndex >= FirstNotVisible);
				Actor->SetActorHiddenInGame(bHidden);
				Actor->SetActorTickEnabled(!bHidden);
				Actor->SetActorEnableCollision(!bHidden);

				// UE-41280 - SetActorEnableCollision doesn't effect child actors, so we must toggle them manually:
				// https://issues.unrealengine.com/issue/UE-41280
				ChildActors.Reset();
				Actor->GetAllChildActors(ChildActors);
				for (int ChildActorIndex = 0; ChildActorIndex < ChildActors.Num(); ++ChildActorIndex)
				{
					ChildActors[ChildActorIndex]->SetActorEnableCollision(!bHidden);
				}

#if WITH_EDITORONLY_DATA
				Actor->SetIsTemporarilyHiddenInEditor(bHidden);
#endif // WITH_EDITORONLY_DATA
			}
		}
	}
}

// This function is slow when moving a collection of a lot of objects. TODO, think of strategies to cache MaterialInstances rather than
// searching for them.
void UUxtScrollingObjectCollectionComponent::UpdateClippingBox()
{
	// Create the clipping box inverse transformation matrix to send into the material.
	// Transposed because HLSL assumes column-major packed matrices by default.
	FTransform Tranform;
	Tranform.SetLocation(GetComponentTransform().TransformPosition(ScrollableBounds.RelativeCenter));
	Tranform.SetRotation(GetComponentRotation().Quaternion());
	Tranform.SetScale3D(ScrollableBounds.Extents * 2);
	FMatrix InverseMatrixTranspose = Tranform.ToInverseMatrixWithScale().GetTransposed();

	const TArray<AActor*>& Actors = GetAttachedActors();

	for (int ActorIndex = 0; ActorIndex < Actors.Num(); ++ActorIndex)
	{
		AActor* Actor = Actors[ActorIndex];

		if (Actor != nullptr)
		{
			TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
			const bool bIncludeFromChildActors = true;
			Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents, bIncludeFromChildActors);

			for (UPrimitiveComponent* const Component : PrimitiveComponents)
			{
				for (int MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
				{
					// CreateAndSetMaterialInstanceDynamic will create a material instance the first time it is called, then use the
					// existing one in future calls.
					UMaterialInstanceDynamic* MaterialInstance = Component->CreateAndSetMaterialInstanceDynamic(MaterialIndex);

					if (MaterialInstance != nullptr)
					{
						static const FName SettingsName = "ClippingBoxSettings";
						MaterialInstance->SetVectorParameterValue(SettingsName, FLinearColor::Red);
						static const FName ColumnName0 = "ClippingBoxTransformColumn0";
						MaterialInstance->SetVectorParameterValue(ColumnName0, InverseMatrixTranspose.GetColumn(0));
						static const FName ColumnName1 = "ClippingBoxTransformColumn1";
						MaterialInstance->SetVectorParameterValue(ColumnName1, InverseMatrixTranspose.GetColumn(1));
						static const FName ColumnName2 = "ClippingBoxTransformColumn2";
						MaterialInstance->SetVectorParameterValue(ColumnName2, InverseMatrixTranspose.GetColumn(2));
						static const FName ColumnName3 = "ClippingBoxTransformColumn3";
						MaterialInstance->SetVectorParameterValue(ColumnName3, InverseMatrixTranspose.GetColumn(3));
					}
				}
			}
		}
	}
}

void UUxtScrollingObjectCollectionComponent::ConfigureBackPlate()
{
	FindOrCreateSubComponent(BackPlateMeshComponent, this);

	BackPlateMeshComponent->SetRelativeLocation(FVector(0.0f, ScrollableBounds.RelativeCenter.Y, ScrollableBounds.RelativeCenter.Z));
	BackPlateMeshComponent->SetRelativeScale3D(
		FVector(UUxtBackPlateComponent::GetDefaultBackPlateDepth(), ScrollableBounds.Extents.Y * 2.0f, ScrollableBounds.Extents.Z * 2.0f) *
		PlatedPadding);
	BackPlateMeshComponent->SetVisibility(bIsPlated);
	BackPlateMeshComponent->SetCollisionEnabled(bIsPlated ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

void UUxtScrollingObjectCollectionComponent::ConfigureBoxComponent()
{
	FindOrCreateSubComponent(BoxComponent, this);

	BoxComponent->SetRelativeLocation(ScrollableBounds.RelativeCenter);
	BoxComponent->SetBoxExtent(ScrollableBounds.Extents);
	BoxComponent->SetCollisionProfileName(CollisionProfile);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

float UUxtScrollingObjectCollectionComponent::GetScrollOffsetFromLocation(const FVector LocalSpaceLocation) const
{
	// We know that the scroll offset is aligned to one of two possible axes in local space.
	const float FVector::*pOffset = (ScrollDirection == EUxtScrollDirection::UpAndDown ? &FVector::Z : &FVector::Y);
	return LocalSpaceLocation.*pOffset;
}

int UUxtScrollingObjectCollectionComponent::GetNumberOfRowsInCollection() const
{
	const int NumActors = GetAttachedActors().Num();
	int NumRows = (NumActors / Tiers);
	// A remainder means that we need an extra row.
	if (NumActors % Tiers > 0)
	{
		NumRows++;
	}
	return NumRows;
}

float UUxtScrollingObjectCollectionComponent::GetSingleCellOffset() const
{
	return (ScrollDirection == EUxtScrollDirection::UpAndDown ? CellSize.Z : CellSize.Y);
}

bool UUxtScrollingObjectCollectionComponent::IsActiveInteraction() const
{
	return PokePointer.IsValid() || FarPointer.IsValid();
}

void UUxtScrollingObjectCollectionComponent::SetScrollDirection(EUxtScrollDirection Direction)
{
	ScrollDirection = Direction;

	RefreshCollection();
}

void UUxtScrollingObjectCollectionComponent::SetIsPlated(bool IsPlated)
{
	bIsPlated = IsPlated;

	ConfigureBackPlate();
}

void UUxtScrollingObjectCollectionComponent::SetPlatedPadding(const FVector Padding)
{
	PlatedPadding = Padding;

	ConfigureBackPlate();
}

void UUxtScrollingObjectCollectionComponent::SetTiers(int32 IncomingTiers)
{
	Tiers = FMath::Max(ScrollingObjectCollectionMinDim, IncomingTiers);

	RefreshCollection();
}

void UUxtScrollingObjectCollectionComponent::SetViewableArea(int32 IncomingViewableArea)
{
	ViewableArea = FMath::Max(ScrollingObjectCollectionMinDim, IncomingViewableArea);

	RefreshCollection();
}

void UUxtScrollingObjectCollectionComponent::SetCollisionProfile(FName Profile)
{
	CollisionProfile = Profile;

	ConfigureBoxComponent();
}

void UUxtScrollingObjectCollectionComponent::SetCellSize(const FVector& Size)
{
	CellSize = Size;

	RefreshCollection();
}

void UUxtScrollingObjectCollectionComponent::PageBy(
	const int32 NumPages, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& callback)
{
	MoveByItems(NumPages * ViewableArea, bAnimate, callback);
}

void UUxtScrollingObjectCollectionComponent::MoveByItems(
	const int32 NumItems, const bool bAnimate, const FUxtScrollingObjectCollectionOnPaginationEnd& Callback)
{
	// In order to keep things simple lets not allow the pagination request if ...
	// ... the collection is already in the middle of another pagination.
	if (PaginationDelta != 0.0f)
	{
		// Call callback with failure reason.
		Callback.ExecuteIfBound(EUxtPaginateResult::Failed_ConcurrentOperation);
		return;
	}

	// ... the collection is currently being interacted with by the user.
	if (IsActiveInteraction())
	{
		// Call callback with failure reason.
		Callback.ExecuteIfBound(EUxtPaginateResult::Failed_ConcurrentInteraction);
		return;
	}

	// What is the target offset that will take us where we want to be?
	// Remember to account for the possibility that we are halfway into a cell
	// Also we should clamp the current net offset to the allowable range to adjust for the
	// edge case where we are mid bounce and don't want that to effect where we end up.
	float MinOffset, MaxOffset;
	GetValidOffsetRange(&MinOffset, &MaxOffset);
	const float EffectiveOffset = FMath::Clamp(GetCurrentNetOffset(), MinOffset, MaxOffset);
	const float SingleCellOffset = GetSingleCellOffset();
	const float CurrentOffsetIntoCell =
		SingleCellOffset * (EffectiveOffset / SingleCellOffset - FMath::FloorToFloat(EffectiveOffset / SingleCellOffset));

	// What is the valid range of offset deltas that can be applied by this pagination?
	// i.e. we don't want the pagination to scroll past the end of the collection.
	float PaginationDestination = EffectiveOffset + NumItems * GetSingleCellOffset() - CurrentOffsetIntoCell;
	PaginationDelta = FMath::Clamp(PaginationDestination, MinOffset, MaxOffset) - GetCurrentNetOffset();

	if (PaginationDelta == 0.0f)
	{
		// The paginate has been clamped to zero and will not finish because it will effectively never start.
		Callback.ExecuteIfBound(EUxtPaginateResult::Success);
		return;
	}

	// We are going to handle not animating by just setting the time to be that of the last key.
	// When we next update the curve will be evaluated at that time and the pagination will end immediately.
	// Saves yet another logical branch to handle just this situation.
	PaginationTime = bAnimate ? 0.0f : PaginationCurve.GetRichCurve()->GetLastKey().Time;

	OnPaginationComplete = Callback;
}

void UUxtScrollingObjectCollectionComponent::AddActorToCollection(AActor* ActorToAdd)
{
	if (ActorToAdd)
	{
		// We only want to do this when playing to make sure we don't serialize the attachment of a instance component to a template
		// component.
		if (HasBegunPlay() && CollectionRootComponent != nullptr)
		{
			ActorToAdd->AttachToComponent(CollectionRootComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

			RefreshCollection();
		}
	}
}

void UUxtScrollingObjectCollectionComponent::RemoveActorFromCollection(AActor* ActorToRemove, bool DestroyActor)
{
	if (ActorToRemove && GetAttachedActors().Contains(ActorToRemove))
	{
		// We only want to do this when playing to make sure we don't serialize the detachment of a instance component from a template
		// component.
		if (HasBegunPlay())
		{
			ActorToRemove->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

			if (DestroyActor)
			{
				ActorToRemove->Destroy();
			}

			RefreshCollection();
		}
	}
}

void UUxtScrollingObjectCollectionComponent::GetValidOffsetRange(float* const Minimum, float* const Maximum) const
{
	*Minimum = 0.0f;
	*Maximum = FMath::Max(GetNumberOfRowsInCollection() - ViewableArea, 0) * GetSingleCellOffset();
}

// TODO, this doesn't account for fast moving clicks. So when moving a finger quickly though the collection may miss pokes.
void UUxtScrollingObjectCollectionComponent::CheckScrollOrClickNearPointer()
{
	UUxtNearPointerComponent* Pointer = PokePointer.Get();
	if (Pointer != nullptr)
	{
		if (AActor* PrimitiveActor = CheckScrollOrClickCommon(Pointer, Pointer->GetGrabPointerTransform().GetLocation()))
		{
			PokeTarget = IUxtCollectionObject::Execute_GetPokeTarget(PrimitiveActor);
			if (PokeTarget)
			{
				IUxtPokeHandler::Execute_OnBeginPoke(PokeTarget.GetObject(), Pointer);
			}
		}
	}
}

void UUxtScrollingObjectCollectionComponent::CheckScrollOrClickFarPointer()
{
	UUxtFarPointerComponent* Pointer = FarPointer.Get();
	if (Pointer != nullptr)
	{
		if (AActor* PrimitiveActor = CheckScrollOrClickCommon(Pointer, Pointer->GetPointerOrigin()))
		{
			FarTarget = IUxtCollectionObject::Execute_GetFarTarget(PrimitiveActor);
			if (FarTarget)
			{
				IUxtFarHandler::Execute_OnFarPressed(FarTarget.GetObject(), Pointer);
			}
		}
	}
}

AActor* UUxtScrollingObjectCollectionComponent::CheckScrollOrClickCommon(
	const UUxtPointerComponent* Pointer, const FVector& WorldSpaceLocation)
{
	if (Pointer && !bHasHitClickMovementThreshold)
	{
		FVector LocalSpaceLocation = LocationWorldToLocal(WorldSpaceLocation);
		float DeltaMove = FMath::Abs(GetScrollOffsetFromLocation(LocalSpaceLocation) - InteractionOrigin);

		if (DeltaMove < ClickMovementThreshold)
		{
			FHitResult Hit;

			// Query for what the pointer is focused on other than the scrolling object actor.
			TArray<UPrimitiveComponent*> IgnoreComponents;
			TArray<AActor*> IgnoreActors;
			IgnoreActors.Add(GetOwner());

			if (Pointer->TraceFromPointer(Hit, IgnoreComponents, IgnoreActors))
			{
				UPrimitiveComponent* Primitive = Hit.GetComponent();

				if (Primitive != nullptr)
				{
					AActor* PrimitiveActor = Primitive->GetOwner();
					if (PrimitiveActor != nullptr)
					{
						if (IsInCollection(PrimitiveActor) &&
							PrimitiveActor->GetClass()->ImplementsInterface(UUxtCollectionObject::StaticClass()))
						{
							return PrimitiveActor;
						}
					}
				}
			}
		}
	}

	return nullptr;
}

bool UUxtScrollingObjectCollectionComponent::IsInCollection(const AActor* Actor) const
{
	const TArray<AActor*>& Actors = GetAttachedActors();

	// Check if the actor is directly attached, then check if any of the actor's parents are attached.
	while (Actor != nullptr)
	{
		if (Actors.Contains(Actor))
		{
			return true;
		}

		Actor = Actor->GetParentActor();
	}

	return false;
}

bool UUxtScrollingObjectCollectionComponent::VerifyAndEvaluatePaginationCurve(const float EvalTime, float* const Output) const
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

bool UUxtScrollingObjectCollectionComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

bool UUxtScrollingObjectCollectionComponent::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

void UUxtScrollingObjectCollectionComponent::OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer)
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
				ScrollOrClickHandle, this, &UUxtScrollingObjectCollectionComponent::CheckScrollOrClickNearPointer, ScrollOrClickTime,
				false);
		}

		// Work around for issue when bReleaseAtScrollBoundary is false. In this case EUxtPokeBehaviour::FrontFace
		// is returned from GetPokeBehaviour_Implementation. This results in a call to IsFrontFacePokeEnded (UxtNearPointerComponent.cpp)
		// from UUxtNearPointerComponent::UpdatePokeInteraction. From a casual reading of that function it appears as if the
		// SphereAABBIntersection test will result in the test failing if the cursor moves out of the side of the box, and not just if the
		// cursor is in front of the front face. Again, this is a very casual reading of the function but it does appear to match the
		// behavior the we are seeing. So to work around this temporarily, we can just pad out the extents of the box in Y and Z directions.
		if (!bReleaseAtScrollBoundary && BoxComponent != nullptr)
		{
			BoxComponent->SetBoxExtent(
				FVector(ScrollableBounds.Extents.X, ScrollableBounds.Extents.Y * 2.0f, ScrollableBounds.Extents.Z * 2.0f));
		}
	}
}

void UUxtScrollingObjectCollectionComponent::OnUpdatePoke_Implementation(UUxtNearPointerComponent* Pointer)
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
		else
		{
			InteractionOffset = 0.0f;
		}
	}
}

void UUxtScrollingObjectCollectionComponent::OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer)
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
		if (BoxComponent != nullptr)
		{
			BoxComponent->SetBoxExtent(ScrollableBounds.Extents);
		}
	}
}

EUxtPokeBehaviour UUxtScrollingObjectCollectionComponent::GetPokeBehaviour_Implementation() const
{
	// Note: See workaround in OnBeginPoke_Implementation and OnEndPoke_Implementation for case where bReleaseAtScrollBoundary == false
	return bReleaseAtScrollBoundary ? EUxtPokeBehaviour::Volume : EUxtPokeBehaviour::FrontFace;
}

bool UUxtScrollingObjectCollectionComponent::GetClosestPoint_Implementation(
	const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const
{
	OutNormal = GetComponentTransform().GetUnitAxis(EAxis::X);

	float NotUsed;
	return FUxtInteractionUtils::GetDefaultClosestPointOnPrimitive(Primitive, Point, OutClosestPoint, NotUsed);
}

bool UUxtScrollingObjectCollectionComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

bool UUxtScrollingObjectCollectionComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return Primitive == BoxComponent;
}

void UUxtScrollingObjectCollectionComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
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
				ScrollOrClickHandle, this, &UUxtScrollingObjectCollectionComponent::CheckScrollOrClickFarPointer, ScrollOrClickTime, false);
		}
	}
}

void UUxtScrollingObjectCollectionComponent::OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer)
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
		else
		{
			InteractionOffset = 0.0f;
		}
	}
}

void UUxtScrollingObjectCollectionComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
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
