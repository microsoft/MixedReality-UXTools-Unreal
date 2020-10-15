// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlComponent.h"

#include "DrawDebugHelpers.h"

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

namespace
{
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
		TEXT("/UXTools/BoundsControl/SM_BoundingBox_FaceHandle.SM_BoundingBox_FaceHandle"));
	FaceAffordanceMesh = FaceAffordanceMeshFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EdgeAffordanceMeshFinder(
		TEXT("/UXTools/BoundsControl/SM_BoundingBox_RotateHandle.SM_BoundingBox_RotateHandle"));
	EdgeAffordanceMesh = EdgeAffordanceMeshFinder.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CornerAffordanceMeshFinder(
		TEXT("/UXTools/BoundsControl/SM_BoundingBox_ScaleHandle.SM_BoundingBox_ScaleHandle"));
	CornerAffordanceMesh = CornerAffordanceMeshFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Finder(TEXT("/UXTools/Materials/MPC_UXSettings"));
	ParameterCollection = Finder.Object;
}

AActor* UUxtBoundsControlComponent::GetBoundsControlActor() const
{
	return BoundsControlActor;
}

const TMap<UPrimitiveComponent*, FUxtAffordanceInstance>& UUxtBoundsControlComponent::GetPrimitiveAffordanceMap()
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
	Bounds = UUxtMathUtilsFunctionLibrary::CalculateNestedActorBoundsInLocalSpace(GetOwner(), true);

	UpdateAffordanceTransforms();
}

void UUxtBoundsControlComponent::CreateAffordances()
{
	if (!IsValid(Config))
	{
		UE_LOG(LogUxtBoundsControl, Error, TEXT("Config asset is invalid"));
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
	BoundsControlGrabbable->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
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
		UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(BoundsControlActor);
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
	if (BoundsControlActor)
	{
		// Copy loc & rot of the owning actor to the bounds control actor
		BoundsControlActor->SetActorLocationAndRotation(GetOwner()->GetActorLocation(), GetOwner()->GetActorRotation());

		for (const auto& Item : PrimitiveAffordanceMap)
		{
			FVector AffordanceLocation;
			FQuat AffordanceRotation;
			Item.Value.Config.GetWorldLocationAndRotation(Bounds, GetOwner()->GetActorTransform(), AffordanceLocation, AffordanceRotation);
			Item.Key->SetWorldLocation(AffordanceLocation);
			Item.Key->SetWorldRotation(AffordanceRotation);
		}
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
		AffordancePrimitive->SetRelativeScale3D(FVector::OneVector * (1.0f + 0.2f * AffordanceInstance.FocusedTransition));
	}
}

bool UUxtBoundsControlComponent::IsAffordanceGrabbed(const FUxtAffordanceInstance* Affordance) const
{
	return GrabbedAffordances.Contains(Affordance);
}

void UUxtBoundsControlComponent::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->GetRootComponent()->TransformUpdated.AddUObject(this, &UUxtBoundsControlComponent::OnActorTransformUpdate);

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
}

void UUxtBoundsControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAffordances();

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

		FBox NewBounds;
		FQuat DeltaRotation;
		ComputeModifiedBounds(AffordanceInstance.Config, GrabPointer, NewBounds, DeltaRotation);

		// Change the actor transform to match the new bounding box.
		// Bounds are not actually changed, they inherit the transform from the actor.
		FTransform BoxTransform;
		if (GetRelativeBoxTransform(NewBounds, InitialBounds, BoxTransform))
		{
			FTransform NewTransform = BoxTransform * InitialTransform;

			FVector Pivot = InitialTransform.TransformPosition(InitialBounds.GetCenter());
			NewTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(NewTransform, DeltaRotation.Rotator(), Pivot);

			FVector MinBox = FVector(MinimumBoundsScale, MinimumBoundsScale, MinimumBoundsScale);
			FVector MaxBox = FVector(MaximumBoundsScale, MaximumBoundsScale, MaximumBoundsScale);

			NewTransform.SetScale3D(NewTransform.GetScale3D().BoundToBox(MinBox, MaxBox));

			GetOwner()->SetActorTransform(NewTransform);
		}
	}
	else if (!GetOwner()->GetActorTransform().Equals(InitialTransform))
	{
		InitialTransform = GetOwner()->GetActorTransform();
	}

	UpdateAffordanceAnimation(DeltaTime);
}

void UUxtBoundsControlComponent::ComputeModifiedBounds(
	const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer, FBox& OutBounds, FQuat& OutDeltaRotation) const
{
	//
	// Look up settings for the affordance

	const FVector AffordanceLoc = Affordance.GetBoundsLocation();
	const FMatrix AffordanceConstraint = Affordance.GetConstraintMatrix(Config ? Config->LockedAxes : 0);

	//
	// Compute grab pointer movement

	const FVector LocalGrabPoint = GrabPointer.LocalGrabPoint.GetTranslation();

	const FVector Target = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabPointer);
	// Note: BoundsControlActor only copies loc & rot of the owning actor.
	// InitialTransform contains the owning actor scale, which has to be ignored.
	const FVector LocalTarget = InitialTransform.InverseTransformPositionNoScale(Target);

	//
	// Compute modified bounding box

	OutBounds = InitialBounds;
	OutDeltaRotation = FQuat::Identity;

	switch (Affordance.Action)
	{
	case EUxtAffordanceAction::Resize:
	{
		FVector LocalDelta = LocalTarget - LocalGrabPoint;
		FVector ConstrainedDelta = AffordanceConstraint.TransformVector(LocalDelta);

		// Influence factors based on location: only move the side the affordance is on
		FVector MaxFactor =
			FVector(AffordanceLoc.X > 0.0f ? 1.0f : 0.0f, AffordanceLoc.Y > 0.0f ? 1.0f : 0.0f, AffordanceLoc.Z > 0.0f ? 1.0f : 0.0f);
		FVector MinFactor = FVector::OneVector - MaxFactor;
		OutBounds.Min += ConstrainedDelta * MinFactor;
		OutBounds.Max += ConstrainedDelta * MaxFactor;
		break;
	}

	case EUxtAffordanceAction::Translate:
	{
		FVector LocalDelta = LocalTarget - LocalGrabPoint;
		FVector ConstrainedDelta = AffordanceConstraint.TransformVector(LocalDelta);

		// All sides moving together
		OutBounds.Min += ConstrainedDelta;
		OutBounds.Max += ConstrainedDelta;
		break;
	}

	case EUxtAffordanceAction::Scale:
	{
		FVector LocalDelta = LocalTarget - LocalGrabPoint;
		FVector ConstrainedDelta = AffordanceConstraint.TransformVector(LocalDelta);

		// Influence factors based on location: move opposing sides in opposite directions
		FVector MaxFactor =
			FVector(AffordanceLoc.X > 0.0f ? 1.0f : -1.0f, AffordanceLoc.Y > 0.0f ? 1.0f : -1.0f, AffordanceLoc.Z > 0.0f ? 1.0f : -1.0f);
		FVector MinFactor = -MaxFactor;
		OutBounds.Min += ConstrainedDelta * MinFactor;
		OutBounds.Max += ConstrainedDelta * MaxFactor;
		break;
	}

	case EUxtAffordanceAction::Rotate:
	{
		FVector LocalCenter = InitialBounds.GetCenter();
		// Apply constraints to the grab and target vectors.
		FVector ConstrainedGrab = AffordanceConstraint.TransformVector(LocalGrabPoint - LocalCenter);
		FVector ConstrainedTarget = AffordanceConstraint.TransformVector(LocalTarget - LocalCenter);
		FQuat BaseRotation = FQuat::FindBetweenVectors(ConstrainedGrab, ConstrainedTarget);
		FQuat InitRot = InitialTransform.GetRotation();
		OutDeltaRotation = InitRot * BaseRotation * InitRot.Inverse();
		break;
	}
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
		GrabbedAffordances.Emplace(AffordanceInstance);
		InitialBounds = Bounds;
		InitialTransform = GetOwner()->GetActorTransform();

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

void UUxtBoundsControlComponent::OnActorTransformUpdate(
	USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	UpdateAffordanceTransforms();
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
