// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlComponent.h"

#include "Components/BoxComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#if WITH_EDITORONLY_DATA
#include "EditorActorFolders.h"
#endif

DEFINE_LOG_CATEGORY(LogUxtBoundsControl);

/** Internal cache that will be used during the interaction with an affordance. */
struct UxtAffordanceInteractionCache
{
	/**
	 * Whether this cache is valid for use.
	 *
	 * It can be false when the opposite affordance can't be found due to misconfiguration, for example.
	 */
	bool IsValid = false;

	/** Initial bounding box at the start of interaction. */
	FBox InitialBounds;

	/** Initial transform of the actor at the start of interaction. */
	FTransform InitialTransform;

	/** Initial diagonal direction (opposite to grabbed affordance). */
	FVector InitialDiagonalDirection;

	/** Initial location of the opposite affordance. */
	FVector InitialOppositeAffordanceLoc;

	/** Initial transform of the grab point (world space) */
	FTransform InitialGrabPointTransform;

	/** Opposite affordance's primitive. Caching here prevents iterating over the map each frame. */
	UPrimitiveComponent* OppositeAffordancePrimitive;
};

namespace
{
	/**
	 * Associates a specific distance threshold to the corresponding scale factor to apply.
	 */
	struct DistanceToScalePair
	{
		float DistanceThreshold;
		float ScaleFactor;
	};

	/**
	 * Maps distance (in cm) to the scale that should be applied to affordance meshes.
	 *
	 * This must be used to keep affordances visible and easy to interact with even when being far away from the actor.
	 *
	 * It might seem silly, since the scale is just dist/100 in all cases at the moment, but using this leaves the door open to make it
	 * customizable or easily change values later on.
	 *
	 * WARNING: Order is important for querying and 0 is not allowed as key.
	 */
	const TArray<DistanceToScalePair> AffordanceDistToScale =
		{{200.0f, 2.0f}, {400.0f, 4.0f}, {600.0f, 6.0f}, {800.0f, 8.0f}, {1000.0f, 10.0f}};

	static FName LeftPositionParam("LeftPointerPosition");
	static FName RightPositionParam("RightPointerPosition");
	static FName OpacityParam("Opacity");
	static FName IsFocusedParam("IsFocused");
	static FName IsActiveParam("IsActive");

	/** Utility function to get the focused primitive */
	UPrimitiveComponent* GetFocusedPrimitive(const UUxtNearPointerComponent* NearPointer)
	{
		FVector ClosestPoint, Normal;
		return NearPointer->GetFocusedGrabPrimitive(ClosestPoint, Normal);
	}

	/** Utility function to get the focused primitive */
	UPrimitiveComponent* GetFocusedPrimitive(const UUxtFarPointerComponent* FarPointer) { return FarPointer->GetHitPrimitive(); }

	/** Utility function to get the focused primitive */
	UPrimitiveComponent* GetFocusedPrimitive(const FUxtGrabPointerData& PointerData)
	{
		return PointerData.NearPointer ? GetFocusedPrimitive(PointerData.NearPointer)
									   : (PointerData.FarPointer ? GetFocusedPrimitive(PointerData.FarPointer) : nullptr);
	}

	FString GetAffordanceBoundsAsString(FUxtAffordanceConfig AffordanceConfig)
	{
		const FVector BoundsLoc = AffordanceConfig.GetBoundsLocation();
		return FString::FromInt(BoundsLoc.X) + "," + FString::FromInt(BoundsLoc.Y) + "," + FString::FromInt(BoundsLoc.Z);
	}

	/**
	 * Finds the affordance primitive that is opposite to the one specified by @ref Config.
	 *
	 * That will be the affordance on the other side of the diagonal that passes through:
	 *  - If !IsFlat, the center of the bounding box.
	 *  - If IsFlat, the center of the front face (X axis).
	 */
	UPrimitiveComponent* GetOppositeAffordance(
		const TMap<UPrimitiveComponent*, FUxtAffordanceInstance>& PrimitiveAffordanceMap, const FUxtAffordanceConfig& Config,
		const bool IsFlat = false)
	{
		FVector OppositeBounds = -Config.GetBoundsLocation();
		if (IsFlat)
		{
			// If flat (2D slate), search for the opposite corner inside the same face (use the original X)
			OppositeBounds.X *= -1;
		}
		for (const auto& AffordancePair : PrimitiveAffordanceMap)
		{
			if (AffordancePair.Value.Config.GetBoundsLocation().Equals(OppositeBounds))
			{
				return AffordancePair.Key;
			}
		}
		return nullptr;
	}

	/**
	 * Finds the normal of the plane that a given affordance should rotate around.
	 *
	 * @param NormalizedAffordanceBounds Affordance bounds normalized to the [-1, 1] range.
	 */
	FVector GetRotationPlaneNormal(const FVector& NormalizedAffordanceLoc)
	{
		FVector RotationPlane = FVector::ZeroVector;

		// Discard corners (at least one component is 0 in all non-corner affordances)
		if (NormalizedAffordanceLoc.X * NormalizedAffordanceLoc.Y * NormalizedAffordanceLoc.Z)
		{
			return RotationPlane;
		}

		// Since rotation affordances are on the edges, the rotation plane slices the object through the center
		if (NormalizedAffordanceLoc.Z == 0)
		{
			RotationPlane = FVector::UpVector;
		}
		else if (NormalizedAffordanceLoc.Y == 0)
		{
			RotationPlane = FVector::RightVector;
		}
		else
		{
			RotationPlane = FVector::ForwardVector;
		}
		return RotationPlane;
	}
} // namespace

UUxtBoundsControlComponent::UUxtBoundsControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;

	static ConstructorHelpers::FObjectFinder<UUxtBoundsControlConfig> ConfigFinder(
		TEXT("/UXTools/BoundsControl/Presets/BoundsControlDefault"));
	Config = ConfigFinder.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> FaceAffordanceMeshFinder(
		TEXT("/UXTools/BoundsControl/SM_BoundingBox_TranslateHandle.SM_BoundingBox_TranslateHandle"));
	FaceAffordanceMesh = FaceAffordanceMeshFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EdgeAffordanceMeshFinder(
		TEXT("/UXTools/BoundsControl/SM_BoundingBox_RotateHandle.SM_BoundingBox_RotateHandle"));
	EdgeAffordanceMesh = EdgeAffordanceMeshFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CornerAffordanceMeshFinder(
		TEXT("/UXTools/BoundsControl/SM_BoundingBox_ScaleHandle.SM_BoundingBox_ScaleHandle"));
	CornerAffordanceMesh = CornerAffordanceMeshFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Finder(TEXT("/UXTools/Materials/MPC_UXSettings"));
	ParameterCollection = Finder.Object;

	InteractionCache = MakeUnique<UxtAffordanceInteractionCache>();
}

UUxtBoundsControlComponent::UUxtBoundsControlComponent(FVTableHelper& Helper)
{
}

UUxtBoundsControlComponent::~UUxtBoundsControlComponent() = default;

AActor* UUxtBoundsControlComponent::GetBoundsControlActor() const
{
	return BoundsControlActor;
}

const TMap<UPrimitiveComponent*, FUxtAffordanceInstance>& UUxtBoundsControlComponent::GetPrimitiveAffordanceMap() const
{
	return PrimitiveAffordanceMap;
}

bool UUxtBoundsControlComponent::GetInitBoundsFromActor() const
{
	return bInitBoundsFromActor;
}

UStaticMesh* UUxtBoundsControlComponent::GetAffordanceKindMesh(EUxtAffordanceKind Kind) const
{
	switch (Kind)
	{
	case EUxtAffordanceKind::Center:
		return CenterAffordanceMesh;
	case EUxtAffordanceKind::Face:
		return FaceAffordanceMesh;
	case EUxtAffordanceKind::Edge:
		return EdgeAffordanceMesh;
	case EUxtAffordanceKind::Corner:
		return CornerAffordanceMesh;
	}

	return nullptr;
}

const FBox& UUxtBoundsControlComponent::GetBounds() const
{
	return Bounds;
}

void UUxtBoundsControlComponent::ComputeBoundsFromComponents()
{
	const USceneComponent* Override = Cast<USceneComponent>(BoundsOverride.GetComponent(GetOwner()));
	const USceneComponent* BoundsTargetComponent = nullptr;
	if (Override)
	{
		BoundsTargetComponent = Override;
	}
	else if (GetOwner())
	{
		BoundsTargetComponent = GetOwner()->GetRootComponent();
	}
	if (BoundsTargetComponent)
	{
		const FTransform BoundsTargetToWorld = BoundsTargetComponent->GetComponentTransform();
		Bounds =
			UUxtMathUtilsFunctionLibrary::CalculateNestedBoundsInGivenSpace(BoundsTargetComponent, BoundsTargetToWorld.Inverse(), true);
	}
	else
	{
		Bounds.Init();
	}
}

UPrimitiveComponent* UUxtBoundsControlComponent::GetAffordancePrimitive(const EUxtAffordancePlacement Placement) const
{
	for (const TPair<UPrimitiveComponent*, FUxtAffordanceInstance>& Pair : PrimitiveAffordanceMap)
	{
		if (Pair.Value.Config.Placement == Placement)
		{
			return Pair.Key;
		}
	}
	return nullptr;
}

void UUxtBoundsControlComponent::CreateAffordances()
{
	if (!IsValid(Config))
	{
		UE_LOG(LogUxtBoundsControl, Error, TEXT("Config asset is invalid"));
		return;
	}
	if (!GetOwner())
	{
		UE_LOG(LogUxtBoundsControl, Error, TEXT("Bounds control component needs owning actor to create affordances"));
		return;
	}

	// Construct the bounds control actor for affordances and grab interaction
	FActorSpawnParameters Params;
	Params.Name = FName(GetOwner()->GetName() + TEXT("_BoundsControl"));
	Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	Params.Owner = GetOwner();
	BoundsControlActor = GetWorld()->SpawnActor<AActor>(Params);
#if WITH_EDITORONLY_DATA
	BoundsControlActor->SetActorLabel(FString::Printf(TEXT("%s %s"), *GetOwner()->GetName(), TEXT("BoundsControl")));
#endif

	USceneComponent* RootComponent = NewObject<USceneComponent>(BoundsControlActor);
	RootComponent->RegisterComponent();
	BoundsControlActor->AddInstanceComponent(RootComponent);
	BoundsControlActor->SetRootComponent(RootComponent);

	BoundsControlGrabbable = NewObject<UUxtGrabTargetComponent>(BoundsControlActor);
	BoundsControlGrabbable->GrabModes = static_cast<int32>(EUxtGrabMode::OneHanded);
	BoundsControlGrabbable->RegisterComponent();
	BoundsControlActor->AddInstanceComponent(BoundsControlGrabbable);

	// All affordances are grabbable through the singular GrabComponent
	BoundsControlGrabbable->OnEnterFarFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceEnterFarFocus);
	BoundsControlGrabbable->OnEnterGrabFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceEnterGrabFocus);
	BoundsControlGrabbable->OnExitFarFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceExitFarFocus);
	BoundsControlGrabbable->OnExitGrabFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceExitGrabFocus);
	BoundsControlGrabbable->OnBeginGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceBeginGrab);
	BoundsControlGrabbable->OnUpdateGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceUpdateGrab);
	BoundsControlGrabbable->OnEndGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceEndGrab);

	for (const FUxtAffordanceConfig& AffordanceConfig : Config->Affordances)
	{
		// Create the mesh component for visuals and collision
		const FName AffordanceName = FName("Affordance_" + GetAffordanceBoundsAsString(AffordanceConfig));
		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(BoundsControlActor, AffordanceName);
		MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
		MeshComponent->RegisterComponent();
		BoundsControlActor->AddInstanceComponent(MeshComponent);
		if (UStaticMesh* AffordanceMesh = GetAffordanceKindMesh(AffordanceConfig.GetAffordanceKind()))
		{
			MeshComponent->SetStaticMesh(AffordanceMesh);
		}

		// Each affordance gets its own dynamic material instance for highlighting
		UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(0);

		// Register the affordance
		FUxtAffordanceInstance AffordanceInstance = {AffordanceConfig, DynamicMaterial};
		PrimitiveAffordanceMap.Add(MeshComponent, AffordanceInstance);
	}
}

void UUxtBoundsControlComponent::DestroyAffordances()
{
	// If config file wasn't valid, it's nullptr
	if (!BoundsControlActor)
	{
		return;
	}

	// If any grab pointers are still active, end the interaction.
	if (BoundsControlGrabbable->GetGrabPointers().Num() > 0)
	{
		// Only one grab at a time supported for now
		check(GrabbedAffordances.Num() == 1);

		const FUxtAffordanceInstance* AffordanceInstance = GrabbedAffordances[0];

		OnManipulationEnded.Broadcast(this, AffordanceInstance->Config, BoundsControlGrabbable);

		// Drop active grab pointers.
		GrabbedAffordances.Empty();
	}

	// Destroy affordances
	PrimitiveAffordanceMap.Empty();
	GetWorld()->DestroyActor(BoundsControlActor);
}

void UUxtBoundsControlComponent::UpdateAffordanceTransforms()
{
	if (!GetOwner())
	{
		return;
	}

	const USceneComponent* const Override = Cast<USceneComponent>(BoundsOverride.GetComponent(GetOwner()));
	const USceneComponent* const BoundsTargetComponent = Override ? Override : GetOwner()->GetRootComponent();
	if (BoundsControlActor)
	{
		const FVector Location = BoundsTargetComponent->GetComponentLocation();
		const FRotator Rotation = BoundsTargetComponent->GetComponentRotation();

		BoundsControlActor->SetActorLocationAndRotation(Location, Rotation);
	}

	const FVector ActorCenterLoc = GetOwner()->GetTransform().TransformPosition(Bounds.GetCenter());
	for (auto& Item : PrimitiveAffordanceMap)
	{
		FVector AffordanceLocation;
		FQuat AffordanceRotation;
		Item.Value.Config.GetWorldLocationAndRotation(
			Bounds, BoundsTargetComponent->GetComponentTransform(), AffordanceLocation, AffordanceRotation);
		Item.Key->SetWorldLocation(AffordanceLocation);
		Item.Key->SetWorldRotation(AffordanceRotation);

		const UPrimitiveComponent* AffordancePrimitive = Item.Key;
		FUxtAffordanceInstance& AffordanceInstance = Item.Value;

		const float DistanceToCenter = FVector::Distance(ActorCenterLoc, AffordanceLocation);
		float ScaleFactor = 1.0f;
		float DistanceThreshold = 0.0f;
		for (const DistanceToScalePair& Pair : AffordanceDistToScale)
		{
			const float PreviousThreshold = DistanceThreshold;
			DistanceThreshold = Pair.DistanceThreshold;
			if (DistanceThreshold > DistanceToCenter)
			{
				// ScaleFactor already holds the previous value, so add the appropriate amount towards the one after
				check(!FMath::IsNearlyZero(DistanceThreshold));
				ScaleFactor += FMath::Lerp(0.0f, Pair.ScaleFactor, DistanceToCenter / DistanceThreshold);
				break;
			}
			else
			{
				ScaleFactor = Pair.ScaleFactor;
			}
		}
		AffordanceInstance.ReferenceRelativeScale = AffordanceInstance.InitialRelativeScale * ScaleFactor;
	}
}

void UUxtBoundsControlComponent::UpdateAffordanceAnimation(float DeltaTime)
{
	// Check hand distance
	bool bHasLeftPointer = false;
	bool bHasRightPointer = false;
	FLinearColor LeftPosition, RightPosition;
	if (ParameterCollection)
	{
		UMaterialParameterCollectionInstance* ParameterCollectionInstance = GetWorld()->GetParameterCollectionInstance(ParameterCollection);
		bHasLeftPointer = ParameterCollectionInstance->GetVectorParameterValue(LeftPositionParam, LeftPosition);
		bHasRightPointer = ParameterCollectionInstance->GetVectorParameterValue(RightPositionParam, RightPosition);
	}

	// Update animation for each affordance
	for (auto& Item : PrimitiveAffordanceMap)
	{
		UPrimitiveComponent* AffordancePrimitive = Item.Key;
		FUxtAffordanceInstance& AffordanceInstance = Item.Value;

		bool bIsVisible = false;
		float Opacity = 0.0f;
		if (bHasLeftPointer || bHasRightPointer)
		{
			float MinDistance;
			if (bHasLeftPointer && bHasRightPointer)
			{
				MinDistance = FMath::Min(
					FVector::Distance(FVector(LeftPosition), AffordancePrimitive->GetComponentLocation()),
					FVector::Distance(FVector(RightPosition), AffordancePrimitive->GetComponentLocation()));
			}
			else if (bHasLeftPointer)
			{
				MinDistance = FVector::Distance(FVector(LeftPosition), AffordancePrimitive->GetComponentLocation());
			}
			else /* bHasRightPointer */
			{
				MinDistance = FVector::Distance(FVector(RightPosition), AffordancePrimitive->GetComponentLocation());
			}

			// If any affordances are being grabbed make sure the grabbed affordace is visible and other affordances are not visible.
			if (GrabbedAffordances.Num() != 0)
			{
				bIsVisible = IsAffordanceGrabbed(&AffordanceInstance);
				Opacity = bIsVisible ? 1.0f : 0.0f;
			}
			else
			{
				// Hide affordances outside the visibility distance
				bIsVisible = MinDistance < AffordanceVisibilityDistance;
				Opacity = FMath::IsNearlyZero(AffordanceVisibilityDistance) ? 0.0f : 1.0f - MinDistance / AffordanceVisibilityDistance;
			}
		}

		// Animate material parameters and scale when focused or grabbed
		const float TransitionDelta = FMath::IsNearlyZero(AffordanceTransitionDuration) ? 1.0f : DeltaTime / AffordanceTransitionDuration;
		const bool bAffordanceIsFocused = AffordanceInstance.FocusCount > 0;
		const bool bAffordanceIsActive = FindGrabPointer(&AffordanceInstance) != nullptr;
		AffordanceInstance.FocusedTransition =
			FMath::Clamp(AffordanceInstance.FocusedTransition + (bAffordanceIsFocused ? TransitionDelta : -TransitionDelta), 0.0f, 1.0f);
		AffordanceInstance.ActiveTransition =
			FMath::Clamp(AffordanceInstance.ActiveTransition + (bAffordanceIsActive ? TransitionDelta : -TransitionDelta), 0.0f, 1.0f);

		AffordancePrimitive->SetHiddenInGame(!bIsVisible);
		if (AffordanceInstance.DynamicMaterial)
		{
			AffordanceInstance.DynamicMaterial->SetScalarParameterValue(OpacityParam, Opacity);
			AffordanceInstance.DynamicMaterial->SetScalarParameterValue(IsFocusedParam, AffordanceInstance.FocusedTransition);
			AffordanceInstance.DynamicMaterial->SetScalarParameterValue(IsActiveParam, AffordanceInstance.ActiveTransition);
		}
		AffordancePrimitive->SetRelativeScale3D(
			AffordanceInstance.ReferenceRelativeScale * (1.0f + 0.2f * AffordanceInstance.FocusedTransition));
	}
}

bool UUxtBoundsControlComponent::IsAffordanceGrabbed(const FUxtAffordanceInstance* Affordance) const
{
	return GrabbedAffordances.Contains(Affordance);
}

void UUxtBoundsControlComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bInitBoundsFromActor)
	{
		ComputeBoundsFromComponents();
	}
	else
	{
		Bounds = FBox(EForceInit::ForceInitToZero);
	}

	CreateAffordances();
	UpdateAffordanceTransforms();
	ResetConstraintsReferenceTransform();

	CreateCollisionBox();
}

void UUxtBoundsControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAffordances();
	// Needs to be destroyed explicitly because it's attached to the owning actor
	CollisionBox->UnregisterComponent();
	CollisionBox->DestroyComponent();

	Super::EndPlay(EndPlayReason);
}

void UUxtBoundsControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GrabbedAffordances.Num() > 0)
	{
		// Get the active affordance data
		// Only one grab at a time supported for now
		check(GrabbedAffordances.Num() == 1);
		const FUxtAffordanceInstance& AffordanceInstance = *GrabbedAffordances[0];
		const FUxtGrabPointerData& GrabPointer = BoundsControlGrabbable->GetGrabPointers()[0];

		if (InteractionCache->IsValid)
		{
			TransformTarget(AffordanceInstance.Config, GrabPointer);
		}
	}
	ComputeBoundsFromComponents();
	UpdateAffordanceTransforms();

	UpdateAffordanceAnimation(DeltaTime);
}

void UUxtBoundsControlComponent::TransformTarget(const FUxtAffordanceConfig& AffordanceConfig, const FUxtGrabPointerData& GrabPointer) const
{
	const FTransform GrabTransform = GrabPointer.GrabPointTransform;

	const bool IsNear = GrabPointer.NearPointer != nullptr;
	check(IsNear != (GrabPointer.FarPointer != nullptr)); // Only one must be nullptr

	FVector ScaleFactor = FVector::OneVector;
	FTransform NewTransform = InteractionCache->InitialTransform;

	switch (AffordanceConfig.GetAction())
	{
	case EUxtAffordanceAction::Translate:
	{
		const FVector CurrentWorldGrabPointLoc = GrabTransform.GetLocation();
		const FVector InitialWorldGrabPointLoc = InteractionCache->InitialGrabPointTransform.GetLocation();

		// Translation axis aligned to actor's transform
		const FVector TranslationAxis = NewTransform.TransformVector(AffordanceConfig.GetBoundsLocation());

		const FVector ProjectedTranslation = (CurrentWorldGrabPointLoc - InitialWorldGrabPointLoc).ProjectOnTo(TranslationAxis);
		NewTransform.AddToTranslation(ProjectedTranslation);
		ApplyConstraints(NewTransform, EUxtTransformMode::Translation, true, IsNear);
		break;
	}
	case EUxtAffordanceAction::Scale:
	{
		// Calculate initial and current distances to the opposite affordance
		const FVector InitialOppositeToGrabPoint =
			InteractionCache->InitialGrabPointTransform.GetLocation() - InteractionCache->InitialOppositeAffordanceLoc;
		const FVector CurrentOppositeToGrabPoint = GrabTransform.GetLocation() - InteractionCache->InitialOppositeAffordanceLoc;

		// Calculate the scale factor as the difference between initial and current distances
		if (Config->bUniformScaling)
		{
			const float InitialDist = FVector::DotProduct(InitialOppositeToGrabPoint, InteractionCache->InitialDiagonalDirection);
			const float CurrentDist = FVector::DotProduct(CurrentOppositeToGrabPoint, InteractionCache->InitialDiagonalDirection);

			const float ScaleFactorUniform = (CurrentDist - InitialDist) / InitialDist;
			ScaleFactor += FVector(ScaleFactorUniform);
		}
		else
		{
			const FTransform& InitialTransform = InteractionCache->InitialTransform;

			ScaleFactor = InitialTransform.InverseTransformVector(CurrentOppositeToGrabPoint) /
						  InitialTransform.InverseTransformVector(InitialOppositeToGrabPoint);
		}
		if (Config->bIsSlate)
		{
			ScaleFactor.X = 1;
		}
		NewTransform.SetScale3D(InteractionCache->InitialTransform.GetScale3D() * ScaleFactor);
		ApplyConstraints(NewTransform, EUxtTransformMode::Scaling, true, IsNear);

		const FVector DiagVec = NewTransform.TransformVector(
			InteractionCache->InitialTransform.InverseTransformVector(InteractionCache->InitialDiagonalDirection));

		// When scaling, leave the opposite corner pinned to its initial location
		NewTransform.SetLocation(InteractionCache->InitialOppositeAffordanceLoc + DiagVec);

		break;
	}
	case EUxtAffordanceAction::Rotate:
	{
		const FVector LocalPivot = InteractionCache->InitialBounds.GetCenter();
		const FVector Pivot = InteractionCache->InitialTransform.TransformPosition(LocalPivot);

		const FVector RotPlaneNormal =
			InteractionCache->InitialTransform.TransformVectorNoScale(GetRotationPlaneNormal(AffordanceConfig.GetBoundsLocation()));

		const FVector InitialLocalGrabLoc = InteractionCache->InitialGrabPointTransform.GetLocation() - Pivot;
		const FVector CurrentLocalGrabLoc = GrabTransform.GetLocation() - Pivot;

		const FVector InitDir = FVector::VectorPlaneProject(InitialLocalGrabLoc, RotPlaneNormal).GetSafeNormal();
		const FVector CurrDir = FVector::VectorPlaneProject(CurrentLocalGrabLoc, RotPlaneNormal).GetSafeNormal();

		FTransform ConstrainedTransform = NewTransform;

		FQuat Swing, Twist;
		FQuat::FindBetween(InitDir, CurrDir).ToSwingTwist(RotPlaneNormal, Swing, Twist);
		ConstrainedTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(ConstrainedTransform, Twist.Rotator(), Pivot);

		ApplyConstraints(ConstrainedTransform, EUxtTransformMode::Rotation, true, IsNear);

		// This might seem redundant, but the constraint would rotate back around actor's transform, not Pivot
		const FRotator ConstrainedDeltaRotator = (ConstrainedTransform.GetRotation() * NewTransform.GetRotation().Inverse()).Rotator();
		NewTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(NewTransform, ConstrainedDeltaRotator, Pivot);

		break;
	}
	}

	if (GetOwner())
	{
		GetOwner()->SetActorTransform(NewTransform);
	}
}

void UUxtBoundsControlComponent::OnAffordanceEnterFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = PrimitiveAffordanceMap.Find(GetFocusedPrimitive(Pointer));
	if (ensure(AffordanceInstance))
	{
		++AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceEnterGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = PrimitiveAffordanceMap.Find(GetFocusedPrimitive(Pointer));
	if (ensure(AffordanceInstance))
	{
		++AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceExitFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = PrimitiveAffordanceMap.Find(GetFocusedPrimitive(Pointer));
	if (ensure(AffordanceInstance))
	{
		--AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceExitGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = PrimitiveAffordanceMap.Find(GetFocusedPrimitive(Pointer));
	if (ensure(AffordanceInstance))
	{
		--AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	FUxtAffordanceInstance* AffordanceInstance = PrimitiveAffordanceMap.Find(GetFocusedPrimitive(GrabPointer));
	check(AffordanceInstance != nullptr);

	// Try to start grabbing the affordance.
	// Only one affordance grab allowed for now.
	if (GrabbedAffordances.Num() == 0)
	{
		NotifyManipulationStarted();

		GrabbedAffordances.Emplace(AffordanceInstance);
		UpdateInteractionCache(AffordanceInstance, GrabPointer);
		ResetConstraintsReferenceTransform();

		OnManipulationStarted.Broadcast(this, AffordanceInstance->Config, Grabbable);
	}
}

void UUxtBoundsControlComponent::OnAffordanceUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
}

void UUxtBoundsControlComponent::OnAffordanceEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	FUxtAffordanceInstance* AffordanceInstance = PrimitiveAffordanceMap.Find(GetFocusedPrimitive(GrabPointer));
	check(AffordanceInstance != nullptr);

	// Release grabbed affordance
	int NumRemoved = GrabbedAffordances.Remove(AffordanceInstance);
	if (NumRemoved > 0)
	{
		OnManipulationEnded.Broadcast(this, AffordanceInstance->Config, Grabbable);
	}
}

void UUxtBoundsControlComponent::OnExternalManipulationStarted()
{
	if (GrabbedAffordances.Num() == 0)
	{
		return;
	}
	check(GrabbedAffordances.Num() == 1);
	BoundsControlGrabbable->ForceEndGrab();
}

const FUxtGrabPointerData* UUxtBoundsControlComponent::FindGrabPointer(const FUxtAffordanceInstance* AffordanceInstance)
{
	int32 Index = GrabbedAffordances.IndexOfByKey(AffordanceInstance);
	return (Index >= 0 ? &BoundsControlGrabbable->GetGrabPointers()[Index] : nullptr);
}

bool UUxtBoundsControlComponent::GetRelativeBoxTransform(const FBox& Box, const FBox& RelativeTo, FTransform& OutTransform)
{
	FVector ExtentA = Box.GetExtent();
	FVector ExtentB = RelativeTo.GetExtent();

	bool bIsValid = !(FMath::IsNearlyZero(ExtentB.X) || FMath::IsNearlyZero(ExtentB.Y) || FMath::IsNearlyZero(ExtentB.Z));
	FVector Scale = bIsValid ? ExtentA / ExtentB : FVector::OneVector;
	OutTransform = FTransform(FRotator::ZeroRotator, Box.GetCenter() - RelativeTo.GetCenter() * Scale, Scale);
	return bIsValid;
}

void UUxtBoundsControlComponent::CreateCollisionBox()
{
	if (AActor* const Owner = GetOwner())
	{
		USceneComponent* const Override = Cast<USceneComponent>(BoundsOverride.GetComponent(Owner));
		USceneComponent* const BoundsRoot = Override ? Override : Owner->GetRootComponent();

		CollisionBox = NewObject<UBoxComponent>(Owner);
		CollisionBox->SetupAttachment(BoundsRoot);
		CollisionBox->RegisterComponent();

		CollisionBox->SetBoxExtent(Bounds.GetExtent());
		CollisionBox->SetWorldTransform(FTransform(Bounds.GetCenter()) * BoundsRoot->GetComponentTransform());

		CollisionBox->SetCollisionProfileName(CollisionProfile);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void UUxtBoundsControlComponent::ResetConstraintsReferenceTransform()
{
	if (GetOwner())
	{
		InitializeConstraints(GetOwner()->GetRootComponent());
	}
}

void UUxtBoundsControlComponent::UpdateInteractionCache(
	const FUxtAffordanceInstance* const AffordanceInstance, const FUxtGrabPointerData& GrabPointerData)
{
	InteractionCache->IsValid = false;

	InteractionCache->InitialBounds = Bounds;
	InteractionCache->InitialTransform = GetOwner() ? GetOwner()->GetActorTransform() : FTransform::Identity;

	InteractionCache->OppositeAffordancePrimitive =
		GetOppositeAffordance(PrimitiveAffordanceMap, AffordanceInstance->Config, Config->bIsSlate);

	if (!InteractionCache->OppositeAffordancePrimitive)
	{
		UE_LOG(
			LogUxtBoundsControl, Error,
			TEXT("Couldn't find opposite affordance. If actor is a 2D slate, please make sure that its face is aligned to the X axis "
				 "and that the 'Is Slate' property is checked in the Bounds Control Config data asset"));
		return;
	}

	InteractionCache->InitialOppositeAffordanceLoc = InteractionCache->OppositeAffordancePrimitive->GetComponentTransform().GetLocation();

	InteractionCache->InitialDiagonalDirection =
		InteractionCache->InitialTransform.GetLocation() - InteractionCache->InitialOppositeAffordanceLoc;

	InteractionCache->InitialGrabPointTransform = GrabPointerData.GrabPointTransform;

	InteractionCache->IsValid = true;
}
