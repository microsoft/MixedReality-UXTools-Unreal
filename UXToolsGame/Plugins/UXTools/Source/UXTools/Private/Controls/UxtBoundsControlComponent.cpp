// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlComponent.h"

#include "DrawDebugHelpers.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#if WITH_EDITORONLY_DATA
#include "EditorActorFolders.h"
#endif

namespace
{
	static FName LeftPositionParam("LeftPointerPosition");
	static FName RightPositionParam("RightPointerPosition");
	static FName OpacityParam("Opacity");
	static FName IsFocusedParam("IsFocused");
	static FName IsActiveParam("IsActive");
} // namespace

UUxtBoundsControlComponent::UUxtBoundsControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;

	static ConstructorHelpers::FClassFinder<AActor> FaceAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultFaceAffordance"));
	FaceAffordanceClass = FaceAffordanceClassFinder.Class;
	static ConstructorHelpers::FClassFinder<AActor> EdgeAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultEdgeAffordance"));
	EdgeAffordanceClass = EdgeAffordanceClassFinder.Class;
	static ConstructorHelpers::FClassFinder<AActor> CornerAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultCornerAffordance"));
	CornerAffordanceClass = CornerAffordanceClassFinder.Class;

	static ConstructorHelpers::FObjectFinder<UUxtBoundsControlConfig> ConfigFinder(
		TEXT("/UXTools/BoundsControl/Presets/BoundsControlDefault"));
	Config = ConfigFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Finder(TEXT("/UXTools/Materials/MPC_UXSettings"));
	ParameterCollection = Finder.Object;
}

const TMap<AActor*, FUxtAffordanceInstance>& UUxtBoundsControlComponent::GetActorAffordanceMap()
{
	return ActorAffordanceMap;
}

TSubclassOf<class AActor> UUxtBoundsControlComponent::GetCenterAffordanceClass() const
{
	return CenterAffordanceClass;
}

TSubclassOf<class AActor> UUxtBoundsControlComponent::GetFaceAffordanceClass() const
{
	return FaceAffordanceClass;
}

TSubclassOf<class AActor> UUxtBoundsControlComponent::GetEdgeAffordanceClass() const
{
	return EdgeAffordanceClass;
}

TSubclassOf<class AActor> UUxtBoundsControlComponent::GetCornerAffordanceClass() const
{
	return CornerAffordanceClass;
}

bool UUxtBoundsControlComponent::GetInitBoundsFromActor() const
{
	return bInitBoundsFromActor;
}

TSubclassOf<class AActor> UUxtBoundsControlComponent::GetAffordanceKindActorClass(EUxtAffordanceKind Kind) const
{
	switch (Kind)
	{
	case EUxtAffordanceKind::Center:
		return CenterAffordanceClass;
	case EUxtAffordanceKind::Face:
		return FaceAffordanceClass;
	case EUxtAffordanceKind::Edge:
		return EdgeAffordanceClass;
	case EUxtAffordanceKind::Corner:
		return CornerAffordanceClass;
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

void UUxtBoundsControlComponent::UpdateAffordanceTransforms()
{
	for (const auto& Item : ActorAffordanceMap)
	{
		FVector AffordanceLocation;
		FQuat AffordanceRotation;
		Item.Value.Config.GetWorldLocationAndRotation(Bounds, GetOwner()->GetActorTransform(), AffordanceLocation, AffordanceRotation);
		Item.Key->SetActorLocation(AffordanceLocation);
		Item.Key->SetActorRotation(AffordanceRotation);
	}
}

void UUxtBoundsControlComponent::UpdateAnimation(float DeltaTime)
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
	for (auto& Item : ActorAffordanceMap)
	{
		AActor* AffordanceActor = Item.Key;
		FUxtAffordanceInstance& AffordanceInstance = Item.Value;

		bool bIsVisible = false;
		float Opacity = 0.0f;
		if (bHasLeftPointer || bHasRightPointer)
		{
			float MinDistance;
			if (bHasLeftPointer && bHasRightPointer)
			{
				MinDistance = FMath::Min(
					FVector::Distance(FVector(LeftPosition), AffordanceActor->GetActorLocation()),
					FVector::Distance(FVector(RightPosition), AffordanceActor->GetActorLocation()));
			}
			else if (bHasLeftPointer)
			{
				MinDistance = FVector::Distance(FVector(LeftPosition), AffordanceActor->GetActorLocation());
			}
			else /* bHasRightPointer */
			{
				MinDistance = FVector::Distance(FVector(RightPosition), AffordanceActor->GetActorLocation());
			}

			// If any affordances are being grabbed make sure the grabbed affordace is visible and other affordances are not visible.
			if (ActiveAffordanceGrabPointers.Num() != 0)
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

		AffordanceActor->SetActorHiddenInGame(!bIsVisible);
		AffordanceActor->SetActorEnableCollision(bIsVisible);
		AffordanceInstance.DynamicMaterial->SetScalarParameterValue(OpacityParam, Opacity);

		// Animate material parameters and scale when focused or grabbed
		const float TransitionDelta = FMath::IsNearlyZero(AffordanceTransitionDuration) ? 1.0f : DeltaTime / AffordanceTransitionDuration;
		const bool bAffordanceIsFocused = AffordanceInstance.FocusCount > 0;
		const bool bAffordanceIsActive = FindGrabPointer(&AffordanceInstance) != nullptr;
		AffordanceInstance.FocusedTransition =
			FMath::Clamp(AffordanceInstance.FocusedTransition + (bAffordanceIsFocused ? TransitionDelta : -TransitionDelta), 0.0f, 1.0f);
		AffordanceInstance.ActiveTransition =
			FMath::Clamp(AffordanceInstance.ActiveTransition + (bAffordanceIsActive ? TransitionDelta : -TransitionDelta), 0.0f, 1.0f);

		AffordanceInstance.DynamicMaterial->SetScalarParameterValue(IsFocusedParam, AffordanceInstance.FocusedTransition);
		AffordanceInstance.DynamicMaterial->SetScalarParameterValue(IsActiveParam, AffordanceInstance.ActiveTransition);

		AffordanceActor->SetActorScale3D(FVector::OneVector * (1.0f + 0.2f * AffordanceInstance.FocusedTransition));
	}
}

bool UUxtBoundsControlComponent::IsAffordanceGrabbed(const FUxtAffordanceInstance* Affordance) const
{
	for (auto& Pair : ActiveAffordanceGrabPointers)
	{
		if (Pair.Key == Affordance)
		{
			return true;
		}
	}

	return false;
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

	//
	// Create affordances

#if WITH_EDITORONLY_DATA
	static UEnum* AffordanceKindEnum = StaticEnum<EUxtAffordanceKind>();
	check(AffordanceKindEnum);

	// Generate a folder in editor builds for better organization of the scene hierarchy
	FName FolderPath;
	if (FActorFolders::IsAvailable())
	{
		FActorFolders& Folders = FActorFolders::Get();
		FolderPath = Folders.GetFolderName(*GetWorld(), GetOwner()->GetFolderPath(), FName(GetOwner()->GetName() + TEXT("_Affordances")));
		Folders.CreateFolder(*GetWorld(), FolderPath);
		Folders.GetFolderProperties(*GetWorld(), FolderPath)->bIsExpanded = false;
	}
#endif

	if (Config)
	{
		for (const FUxtAffordanceConfig& AffordanceConfig : Config->Affordances)
		{
			TSubclassOf<AActor> AffordanceClass = GetAffordanceKindActorClass(AffordanceConfig.GetAffordanceKind());
			if (IsValid(AffordanceClass))
			{
				FActorSpawnParameters Params;
				Params.Name = FName(GetOwner()->GetName() + TEXT("_Affordance"));
				Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
				Params.Owner = GetOwner();
				AActor* AffordanceActor = GetWorld()->SpawnActor<AActor>(AffordanceClass, Params);

				if (AffordanceActor != nullptr)
				{
					UMaterialInstanceDynamic* DynamicMaterial = nullptr;
					if (UMeshComponent* MeshComponent = AffordanceActor->FindComponentByClass<UMeshComponent>())
					{
						DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(0);
					}

					FUxtAffordanceInstance AffordanceInstance = {AffordanceConfig, DynamicMaterial};
					ActorAffordanceMap.Add(AffordanceActor, AffordanceInstance);

					UUxtGrabTargetComponent* Grabbable = AffordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
					if (Grabbable != nullptr)
					{
						Grabbable->OnEnterFarFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceEnterFarFocus);
						Grabbable->OnEnterGrabFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceEnterGrabFocus);
						Grabbable->OnExitFarFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceExitFarFocus);
						Grabbable->OnExitGrabFocus.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceExitGrabFocus);

						Grabbable->OnBeginGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceBeginGrab);
						Grabbable->OnUpdateGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceUpdateGrab);
						Grabbable->OnEndGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnAffordanceEndGrab);
					}

#if WITH_EDITORONLY_DATA
					if (FActorFolders::IsAvailable())
					{
						FVector BoundsLocation = AffordanceConfig.GetBoundsLocation();
						AffordanceActor->SetActorLabel(FString::Printf(
							TEXT("%s %s (%.0f %.0f %.0f)"), *GetOwner()->GetName(),
							*AffordanceKindEnum->GetDisplayNameTextByValue((int64)AffordanceConfig.GetAffordanceKind()).ToString(),
							BoundsLocation.X, BoundsLocation.Y, BoundsLocation.Z));
						AffordanceActor->SetFolderPath_Recursively(FolderPath);
					}
#endif
				}
			}
		}
	}

	UpdateAffordanceTransforms();
}

void UUxtBoundsControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// If any grab pointers are still active, end the interaction.
	if (ActiveAffordanceGrabPointers.Num() > 0)
	{
		// Only one grab at a time supported for now
		check(ActiveAffordanceGrabPointers.Num() == 1);

		const FUxtAffordanceInstance* AffordanceInstance = ActiveAffordanceGrabPointers[0].Key;
		const FUxtGrabPointerData& GrabPointer = ActiveAffordanceGrabPointers[0].Value;

		// Find the grab target in use by this pointer
		UUxtGrabTargetComponent* Grabbable = nullptr;
		for (const auto& Item : ActorAffordanceMap)
		{
			if (&Item.Value == AffordanceInstance)
			{
				AActor* AffordanceActor = Item.Key;
				Grabbable = AffordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
				break;
			}
		}
		check(Grabbable != nullptr);

		OnManipulationEnded.Broadcast(this, AffordanceInstance->Config, Grabbable);

		// Drop active grab pointers.
		ActiveAffordanceGrabPointers.Empty();
	}

	// Destroy affordances
	for (const auto& Item : ActorAffordanceMap)
	{
		if (!Item.Key->IsActorBeingDestroyed() && Item.Key->GetWorld() != nullptr)
		{
			GetWorld()->DestroyActor(Item.Key);
		}
	}
	ActorAffordanceMap.Empty();

	Super::EndPlay(EndPlayReason);
}

void UUxtBoundsControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveAffordanceGrabPointers.Num() > 0)
	{
		// Get the active affordance data
		// Only one grab at a time supported for now
		check(ActiveAffordanceGrabPointers.Num() == 1);
		const FUxtAffordanceInstance& AffordanceInstance = *ActiveAffordanceGrabPointers[0].Key;
		const FUxtGrabPointerData& GrabPointer = ActiveAffordanceGrabPointers[0].Value;

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

	UpdateAnimation(DeltaTime);
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
	const FVector GrabPoint = InitialTransform.TransformPosition(LocalGrabPoint);

	const FVector Target = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabPointer);
	const FVector LocalTarget = InitialTransform.InverseTransformPosition(Target);

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
	FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	if (ensure(AffordanceInstance))
	{
		++AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceEnterGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	if (ensure(AffordanceInstance))
	{
		++AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceExitFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	if (ensure(AffordanceInstance))
	{
		--AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceExitGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer)
{
	FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	if (ensure(AffordanceInstance))
	{
		--AffordanceInstance->FocusCount;
	}
}

void UUxtBoundsControlComponent::OnAffordanceBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(AffordanceInstance != nullptr);

	FUxtGrabPointerData BoundsGrabPointer;
	BoundsGrabPointer.NearPointer = GrabPointer.NearPointer;
	BoundsGrabPointer.GrabPointTransform = GrabPointer.GrabPointTransform;
	BoundsGrabPointer.StartTime = GrabPointer.StartTime;
	// Transform into the bbox actor space
	FTransform RelativeTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
	BoundsGrabPointer.LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(RelativeTransform, GrabPointer);

	if (TryActivateGrabPointer(AffordanceInstance, BoundsGrabPointer))
	{
		OnManipulationStarted.Broadcast(this, AffordanceInstance->Config, Grabbable);
	}
}

void UUxtBoundsControlComponent::OnAffordanceUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(AffordanceInstance != nullptr);

	FUxtGrabPointerData* BoundsGrabPointerPtr = FindGrabPointer(AffordanceInstance);
	// Only the first grabbing pointer is supported by bounding box at this point.
	// Other pointers may still be grabbing, but will not have a grab pointer entry.
	if (BoundsGrabPointerPtr)
	{
		BoundsGrabPointerPtr->NearPointer = GrabPointer.NearPointer;
		BoundsGrabPointerPtr->GrabPointTransform = GrabPointer.GrabPointTransform;
		BoundsGrabPointerPtr->StartTime = GrabPointer.StartTime;
		// Transform into the bbox actor space
		FTransform RelativeTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
		BoundsGrabPointerPtr->LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(RelativeTransform, GrabPointer);
	}
}

void UUxtBoundsControlComponent::OnAffordanceEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtAffordanceInstance* AffordanceInstance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(AffordanceInstance != nullptr);

	if (TryReleaseGrabPointer(AffordanceInstance))
	{
		OnManipulationEnded.Broadcast(this, AffordanceInstance->Config, Grabbable);
	}
}

void UUxtBoundsControlComponent::OnActorTransformUpdate(
	USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	UpdateAffordanceTransforms();
}

bool UUxtBoundsControlComponent::TryActivateGrabPointer(
	const FUxtAffordanceInstance* AffordanceInstance, const FUxtGrabPointerData& GrabPointer)
{
	if (ActiveAffordanceGrabPointers.Num() == 0)
	{
		ActiveAffordanceGrabPointers.Emplace(AffordanceInstance, GrabPointer);
		InitialBounds = Bounds;
		InitialTransform = GetOwner()->GetActorTransform();
		return true;
	}
	return false;
}

bool UUxtBoundsControlComponent::TryReleaseGrabPointer(const FUxtAffordanceInstance* AffordanceInstance)
{
	int NumRemoved =
		ActiveAffordanceGrabPointers.RemoveAll([AffordanceInstance](const TPair<const FUxtAffordanceInstance*, FUxtGrabPointerData>& item) {
			return item.Key == AffordanceInstance;
		});
	return NumRemoved > 0;
}

FUxtGrabPointerData* UUxtBoundsControlComponent::FindGrabPointer(const FUxtAffordanceInstance* AffordanceInstance)
{
	for (auto& KeyValuePair : ActiveAffordanceGrabPointers)
	{
		if (KeyValuePair.Key == AffordanceInstance)
		{
			return &KeyValuePair.Value;
		}
	}
	return nullptr;
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
