// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlComponent.h"

#include "DrawDebugHelpers.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

#if WITH_EDITORONLY_DATA
#include "EditorActorFolders.h"
#endif

UUxtBoundsControlComponent::UUxtBoundsControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Tick post-physics to ensure the UxtGenericManipulatorComponent ticks before this one.
	// This prevents the affordances from lagging by one frame when used with a manipulator and also allows them to react to other external
	// changes.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bAutoActivate = true;

	static ConstructorHelpers::FClassFinder<AActor> FaceAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultFaceAffordance"));
	FaceAffordanceClass = FaceAffordanceClassFinder.Class;
	static ConstructorHelpers::FClassFinder<AActor> EdgeAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultEdgeAffordance"));
	EdgeAffordanceClass = EdgeAffordanceClassFinder.Class;
	static ConstructorHelpers::FClassFinder<AActor> CornerAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultCornerAffordance"));
	CornerAffordanceClass = CornerAffordanceClassFinder.Class;
	MinimumBoundsScale = 0.1f;
	MaximumBoundsScale = 5.0f;

	static ConstructorHelpers::FObjectFinder<UUxtBoundsControlConfig> ConfigFinder(
		TEXT("/UXTools/BoundsControl/Presets/BoundsControlDefault"));
	Config = ConfigFinder.Object;
}

const TMap<AActor*, const FUxtAffordanceConfig*>& UUxtBoundsControlComponent::GetActorAffordanceMap()
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

float UUxtBoundsControlComponent::GetMaximumBoundsScale() const
{
	return MaximumBoundsScale;
}

void UUxtBoundsControlComponent::SetMaximumBoundsScale(float Value)
{
	MaximumBoundsScale = Value;
}

void UUxtBoundsControlComponent::SetMinimumBoundsScale(float Value)
{
	MinimumBoundsScale = Value;
}

float UUxtBoundsControlComponent::GetMinimumBoundsScale() const
{
	return MinimumBoundsScale;
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
		FTransform AffordanceTransform = Item.Value->GetWorldTransform(Bounds, GetOwner()->GetActorTransform());
		Item.Key->SetActorTransform(AffordanceTransform);
	}
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
		for (const FUxtAffordanceConfig& Affordance : Config->Affordances)
		{
			TSubclassOf<AActor> AffordanceClass = GetAffordanceKindActorClass(Affordance.GetAffordanceKind());
			if (IsValid(AffordanceClass))
			{
				FActorSpawnParameters Params;
				Params.Name = FName(GetOwner()->GetName() + TEXT("_Affordance"));
				Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
				Params.Owner = GetOwner();
				AActor* AffordanceActor = GetWorld()->SpawnActor<AActor>(AffordanceClass, Params);

				if (AffordanceActor != nullptr)
				{
					ActorAffordanceMap.Add(AffordanceActor, &Affordance);

					UUxtGrabTargetComponent* Grabbable = AffordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
					if (Grabbable != nullptr)
					{
						Grabbable->OnBeginGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnPointerBeginGrab);
						Grabbable->OnUpdateGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnPointerUpdateGrab);
						Grabbable->OnEndGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnPointerEndGrab);
					}

#if WITH_EDITORONLY_DATA
					if (FActorFolders::IsAvailable())
					{
						FVector BoundsLocation = Affordance.GetBoundsLocation();
						AffordanceActor->SetActorLabel(FString::Printf(
							TEXT("%s %s (%.0f %.0f %.0f)"), *GetOwner()->GetName(),
							*AffordanceKindEnum->GetDisplayNameTextByValue((int64)Affordance.GetAffordanceKind()).ToString(),
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

		const FUxtAffordanceConfig* AffordanceInfo = ActiveAffordanceGrabPointers[0].Key;
		const FUxtGrabPointerData& GrabPointer = ActiveAffordanceGrabPointers[0].Value;

		// Find the grab target in use by this pointer
		UUxtGrabTargetComponent* Grabbable = nullptr;
		for (const auto& Item : ActorAffordanceMap)
		{
			if (Item.Value == AffordanceInfo)
			{
				AActor* AffordanceActor = Item.Key;
				Grabbable = AffordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
				break;
			}
		}
		check(Grabbable != nullptr);

		OnManipulationEnded.Broadcast(this, *AffordanceInfo, Grabbable);

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
		const FUxtAffordanceConfig& Affordance = *ActiveAffordanceGrabPointers[0].Key;
		const FUxtGrabPointerData& GrabPointer = ActiveAffordanceGrabPointers[0].Value;

		FBox NewBounds;
		FQuat DeltaRotation;
		ComputeModifiedBounds(Affordance, GrabPointer, NewBounds, DeltaRotation);

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

		UpdateAffordanceTransforms();
	}
	else if (!GetOwner()->GetActorTransform().Equals(InitialTransform))
	{
		InitialTransform = GetOwner()->GetActorTransform();
		UpdateAffordanceTransforms();
	}
}

void UUxtBoundsControlComponent::ComputeModifiedBounds(
	const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer, FBox& OutBounds, FQuat& OutDeltaRotation) const
{
	//
	// Look up settings for the affordance

	const FVector AffordanceLoc = Affordance.GetBoundsLocation();
	const FMatrix AffordanceConstraint = Affordance.GetConstraintMatrix();

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
		FVector MinFactor = (-AffordanceLoc).ComponentMax(FVector::ZeroVector);
		FVector MaxFactor = AffordanceLoc.ComponentMax(FVector::ZeroVector);
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
		FVector MinFactor = -AffordanceLoc;
		FVector MaxFactor = AffordanceLoc;
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

void UUxtBoundsControlComponent::OnPointerBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtAffordanceConfig** AffordancePtr = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(AffordancePtr != nullptr);

	FUxtGrabPointerData BoundsGrabPointer;
	BoundsGrabPointer.NearPointer = GrabPointer.NearPointer;
	BoundsGrabPointer.GrabPointTransform = GrabPointer.GrabPointTransform;
	BoundsGrabPointer.StartTime = GrabPointer.StartTime;
	// Transform into the bbox actor space
	FTransform RelativeTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
	BoundsGrabPointer.LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(RelativeTransform, GrabPointer);

	if (TryActivateGrabPointer(**AffordancePtr, BoundsGrabPointer))
	{
		OnManipulationStarted.Broadcast(this, **AffordancePtr, Grabbable);
	}
}

void UUxtBoundsControlComponent::OnPointerUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtAffordanceConfig** AffordancePtr = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(AffordancePtr != nullptr);

	FUxtGrabPointerData* BoundsGrabPointerPtr = FindGrabPointer(**AffordancePtr);
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

void UUxtBoundsControlComponent::OnPointerEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtAffordanceConfig** AffordancePtr = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(AffordancePtr != nullptr);

	if (TryReleaseGrabPointer(**AffordancePtr))
	{
		OnManipulationEnded.Broadcast(this, **AffordancePtr, Grabbable);
	}
}

bool UUxtBoundsControlComponent::TryActivateGrabPointer(const FUxtAffordanceConfig& Affordance, const FUxtGrabPointerData& GrabPointer)
{
	if (ActiveAffordanceGrabPointers.Num() == 0)
	{
		ActiveAffordanceGrabPointers.Emplace(&Affordance, GrabPointer);
		InitialBounds = Bounds;
		InitialTransform = GetOwner()->GetActorTransform();
		return true;
	}
	return false;
}

bool UUxtBoundsControlComponent::TryReleaseGrabPointer(const FUxtAffordanceConfig& Affordance)
{
	int NumRemoved = ActiveAffordanceGrabPointers.RemoveAll(
		[&Affordance](const TPair<const FUxtAffordanceConfig*, FUxtGrabPointerData>& item) { return item.Key == &Affordance; });
	return NumRemoved > 0;
}

FUxtGrabPointerData* UUxtBoundsControlComponent::FindGrabPointer(const FUxtAffordanceConfig& Affordance)
{
	for (auto& KeyValuePair : ActiveAffordanceGrabPointers)
	{
		if (KeyValuePair.Key == &Affordance)
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
