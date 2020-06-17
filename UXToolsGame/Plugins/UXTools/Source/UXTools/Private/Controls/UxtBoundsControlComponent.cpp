// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundsControlComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
#include "EditorActorFolders.h"
#endif


static FBox CalculateNestedActorBoundsInGivenSpace(const AActor* Actor, const FTransform& WorldToCalcSpace, bool bNonColliding)
{
	FBox Box(ForceInit);

	for (const UActorComponent* ActorComponent : Actor->GetComponents())
	{
		if (!ActorComponent->IsRegistered())
		{
			continue;
		}

		if (const UPrimitiveComponent* PrimitiveComponent = Cast<const UPrimitiveComponent>(ActorComponent))
		{
			// Only use collidable components to find collision bounding box.
			if (bNonColliding || PrimitiveComponent->IsCollisionEnabled())
			{
				const FTransform& ComponentToWorld = PrimitiveComponent->GetComponentTransform();
				const FTransform ComponentToCalcSpace = ComponentToWorld * WorldToCalcSpace;

				const FBoxSphereBounds ComponentBoundsCalcSpace = PrimitiveComponent->CalcBounds(ComponentToCalcSpace);
				const FBox ComponentBox = ComponentBoundsCalcSpace.GetBox();

				Box += ComponentBox;
			}
		}

		if (const UChildActorComponent* ChildActor = Cast<const UChildActorComponent>(ActorComponent))
		{
			if (const AActor* NestedActor = ChildActor->GetChildActor())
			{
				Box += CalculateNestedActorBoundsInGivenSpace(NestedActor, WorldToCalcSpace, bNonColliding);
			}
		}
	}

	return Box;
}

static FBox CalculateNestedActorBoundsInLocalSpace(const AActor* Actor, bool bNonColliding)
{
	const FTransform& ActorToWorld = Actor->GetTransform();
	const FTransform WorldToActor = ActorToWorld.Inverse();

	return CalculateNestedActorBoundsInGivenSpace(Actor, WorldToActor, true);
}

FTransform FUxtBoundsControlAffordanceInfo::GetWorldTransform(const FBox &Bounds, const FTransform &RootTransform) const
{
	FVector Location = Bounds.GetCenter() + Bounds.GetExtent() * BoundsLocation;
	FRotator Rotation = BoundsRotation;
	FVector Scale = FVector::OneVector;

	return FTransform(RootTransform.TransformRotation(FQuat(Rotation)), RootTransform.TransformPosition(Location), Scale);
}


UUxtBoundsControlComponent::UUxtBoundsControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	static ConstructorHelpers::FClassFinder<AActor> FaceAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultFaceAffordance"));
	FaceAffordanceClass = FaceAffordanceClassFinder.Class;
	static ConstructorHelpers::FClassFinder<AActor> EdgeAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultEdgeAffordance"));
	EdgeAffordanceClass = EdgeAffordanceClassFinder.Class;
	static ConstructorHelpers::FClassFinder<AActor> CornerAffordanceClassFinder(TEXT("/UXTools/BoundsControl/BP_DefaultCornerAffordance"));
	CornerAffordanceClass = CornerAffordanceClassFinder.Class;
}

const TMap<AActor*, const FUxtBoundsControlAffordanceInfo*>& UUxtBoundsControlComponent::GetActorAffordanceMap()
{
	return ActorAffordanceMap;
}

const TArray<FUxtBoundsControlAffordanceInfo>& UUxtBoundsControlComponent::GetCustomAffordances() const
{
	return CustomAffordances;
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

bool UUxtBoundsControlComponent::UseCustomAffordances() const
{
	return bUseCustomAffordances;
}

EUxtBoundsControlPreset UUxtBoundsControlComponent::GetPreset() const
{
	return Preset;
}

bool UUxtBoundsControlComponent::GetInitBoundsFromActor() const
{
	return bInitBoundsFromActor;
}

const TArray<FUxtBoundsControlAffordanceInfo>& UUxtBoundsControlComponent::GetUsedAffordances() const
{
	if (bUseCustomAffordances)
	{
		return CustomAffordances;
	}
	else
	{
		return FUxtBoundsControlPresetUtils::GetPresetAffordances(Preset);
	}
}

TSubclassOf<class AActor> UUxtBoundsControlComponent::GetAffordanceKindActorClass(EUxtBoundsControlAffordanceKind Kind) const
{
	switch (Kind)
	{
	case EUxtBoundsControlAffordanceKind::Center:	return CenterAffordanceClass;
	case EUxtBoundsControlAffordanceKind::Face:	return FaceAffordanceClass;
	case EUxtBoundsControlAffordanceKind::Edge:	return EdgeAffordanceClass;
	case EUxtBoundsControlAffordanceKind::Corner:	return CornerAffordanceClass;
	}

	return nullptr;
}

const FBox& UUxtBoundsControlComponent::GetBounds() const
{
	return Bounds;
}

void UUxtBoundsControlComponent::ComputeBoundsFromComponents()
{
	Bounds = CalculateNestedActorBoundsInLocalSpace(GetOwner(), true);

	UpdateAffordanceTransforms();
}

void UUxtBoundsControlComponent::UpdateAffordanceTransforms()
{
	for (const auto &item : ActorAffordanceMap)
	{
		FTransform affordanceTransform = item.Value->GetWorldTransform(Bounds, GetOwner()->GetActorTransform());
		item.Key->SetActorTransform(affordanceTransform);
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
	
#if WITH_EDITOR
	static UEnum* AffordanceKindEnum = StaticEnum<EUxtBoundsControlAffordanceKind>();
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

	const auto &usedAffordances = GetUsedAffordances();
	for (const FUxtBoundsControlAffordanceInfo &affordance : usedAffordances)
	{
		auto affordanceClass = (affordance.ActorClass != nullptr ? affordance.ActorClass : GetAffordanceKindActorClass(affordance.Kind));
		if (IsValid(affordanceClass))
		{
			FActorSpawnParameters Params;
			Params.Name = FName(GetOwner()->GetName() + TEXT("_Affordance"));
			Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
			Params.Owner = GetOwner();
			AActor *affordanceActor = GetWorld()->SpawnActor<AActor>(affordanceClass, Params);

			if (affordanceActor != nullptr)
			{
				ActorAffordanceMap.Add(affordanceActor, &affordance);

				UUxtGrabTargetComponent *grabbable = affordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
				if (grabbable != nullptr)
				{
					grabbable->OnBeginGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnPointerBeginGrab);
					grabbable->OnUpdateGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnPointerUpdateGrab);
					grabbable->OnEndGrab.AddDynamic(this, &UUxtBoundsControlComponent::OnPointerEndGrab);
				}

#if WITH_EDITOR
				if (FActorFolders::IsAvailable())
				{
					affordanceActor->SetActorLabel(FString::Printf(TEXT("%s %s (%.0f %.0f %.0f)"),
						*GetOwner()->GetName(),
						*AffordanceKindEnum->GetDisplayNameTextByValue((int64)affordance.Kind).ToString(),
						affordance.BoundsLocation.X, affordance.BoundsLocation.Y, affordance.BoundsLocation.Z
						));
					affordanceActor->SetFolderPath_Recursively(FolderPath);
				}
#endif
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

		const FUxtBoundsControlAffordanceInfo* affordanceInfo = ActiveAffordanceGrabPointers[0].Key;
		const FUxtGrabPointerData& grabPointer = ActiveAffordanceGrabPointers[0].Value;

		// Find the grab target in use by this pointer
		UUxtGrabTargetComponent* grabbable = nullptr;
		for (auto item : ActorAffordanceMap)
		{
			if (item.Value == affordanceInfo)
			{
				AActor* affordanceActor = item.Key;
				grabbable = affordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
				break;
			}
		}
		check(grabbable != nullptr);

		OnManipulationEnded.Broadcast(this, *affordanceInfo, grabbable);

		// Drop active grab pointers.
		ActiveAffordanceGrabPointers.Empty();
	}

	// Destroy affordances
	for (const auto &item : ActorAffordanceMap)
	{
		if(!item.Key->IsActorBeingDestroyed() && item.Key->GetWorld() != nullptr )
		{
			GetWorld()->DestroyActor(item.Key);
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
		const FUxtBoundsControlAffordanceInfo &affordance = *ActiveAffordanceGrabPointers[0].Key;
		const FUxtGrabPointerData &grabPointer = ActiveAffordanceGrabPointers[0].Value;

		FBox newBounds;
		FQuat deltaRotation;
		ComputeModifiedBounds(affordance, grabPointer, newBounds, deltaRotation);

		// Change the actor transform to match the new bounding box.
		// Bounds are not actually changed, they inherit the transform from the actor.
		FTransform boxTransform;
		if (GetRelativeBoxTransform(newBounds, InitialBounds, boxTransform))
		{
			FTransform newTransform = boxTransform * InitialTransform;

			FVector pivot = InitialTransform.TransformPosition(InitialBounds.GetCenter());
			newTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(newTransform, deltaRotation.Rotator(), pivot);
			GetOwner()->SetActorTransform(newTransform);
		}

		UpdateAffordanceTransforms();
	}
	else if (!GetOwner()->GetActorTransform().Equals(InitialTransform))
	{
		InitialTransform = GetOwner()->GetActorTransform();
		UpdateAffordanceTransforms();
	}
}

void UUxtBoundsControlComponent::ComputeModifiedBounds(const FUxtBoundsControlAffordanceInfo &Affordance, const FUxtGrabPointerData &GrabPointer, FBox &OutBounds, FQuat &OutDeltaRotation) const
{
	//
	// Look up settings for the affordance

	const FVector affordanceLoc = Affordance.BoundsLocation;
	const FMatrix affordanceConstraint = Affordance.ConstraintMatrix;

	//
	// Compute grab pointer movement

	const FVector localGrabPoint = GrabPointer.LocalGrabPoint.GetTranslation();
	const FVector grabPoint = InitialTransform.TransformPosition(localGrabPoint);

	const FVector target = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GrabPointer);
	const FVector localTarget = InitialTransform.InverseTransformPosition(target);

	//
	// Compute modified bounding box

	OutBounds = InitialBounds;
	OutDeltaRotation = FQuat::Identity;

	switch (Affordance.Action)
	{
	case EUxtBoundsControlAffordanceAction::Resize:
	{

		FVector localDelta = localTarget - localGrabPoint;
		FVector constrainedDelta = affordanceConstraint.TransformVector(localDelta);

		// Influence factors based on location: only move the side the affordance is on
		FVector minFactor = (-affordanceLoc).ComponentMax(FVector::ZeroVector);
		FVector maxFactor = affordanceLoc.ComponentMax(FVector::ZeroVector);
		OutBounds.Min += constrainedDelta * minFactor;
		OutBounds.Max += constrainedDelta * maxFactor;
		break;
	}

	case EUxtBoundsControlAffordanceAction::Translate:
	{
		FVector localDelta = localTarget - localGrabPoint;
		FVector constrainedDelta = affordanceConstraint.TransformVector(localDelta);

		// All sides moving together
		OutBounds.Min += constrainedDelta;
		OutBounds.Max += constrainedDelta;
		break;
	}

	case EUxtBoundsControlAffordanceAction::Scale:
	{
		FVector localDelta = localTarget - localGrabPoint;
		FVector constrainedDelta = affordanceConstraint.TransformVector(localDelta);

		// Influence factors based on location: move opposing sides in opposite directions
		FVector minFactor = -affordanceLoc;
		FVector maxFactor = affordanceLoc;
		OutBounds.Min += constrainedDelta * minFactor;
		OutBounds.Max += constrainedDelta * maxFactor;
		break;
	}

	case EUxtBoundsControlAffordanceAction::Rotate:
	{
		FVector localCenter = InitialBounds.GetCenter();
		// Apply constraints to the grab and target vectors.
		FVector constrainedGrab = affordanceConstraint.TransformVector(localGrabPoint - localCenter);
		FVector constrainedTarget = affordanceConstraint.TransformVector(localTarget - localCenter);
		FQuat baseRotation = FQuat::FindBetweenVectors(constrainedGrab, constrainedTarget);
		FQuat initRot = InitialTransform.GetRotation();
		OutDeltaRotation = initRot * baseRotation * initRot.Inverse();
		break;
	}
	}
}

void UUxtBoundsControlComponent::OnPointerBeginGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtBoundsControlAffordanceInfo **pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	FUxtGrabPointerData bboxGrabPointer;
	bboxGrabPointer.NearPointer = GrabPointer.NearPointer;
	bboxGrabPointer.GrabPointTransform = GrabPointer.GrabPointTransform;
	bboxGrabPointer.StartTime = GrabPointer.StartTime;
	// Transform into the bbox actor space
	FTransform relTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
	bboxGrabPointer.LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(relTransform, GrabPointer);

	if (TryActivateGrabPointer(**pAffordance, bboxGrabPointer))
	{
		OnManipulationStarted.Broadcast(this, **pAffordance, Grabbable);
	}
}

void UUxtBoundsControlComponent::OnPointerUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtBoundsControlAffordanceInfo** pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	FUxtGrabPointerData* pBBoxGrabPointer = FindGrabPointer(**pAffordance);
	// Only the first grabbing pointer is supported by bounding box at this point.
	// Other pointers may still be grabbing, but will not have a grab pointer entry.
	if (pBBoxGrabPointer)
	{
		pBBoxGrabPointer->NearPointer = GrabPointer.NearPointer;
		pBBoxGrabPointer->GrabPointTransform = GrabPointer.GrabPointTransform;
		pBBoxGrabPointer->StartTime = GrabPointer.StartTime;
		// Transform into the bbox actor space
		FTransform relTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
		pBBoxGrabPointer->LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(relTransform, GrabPointer);
	}
}

void UUxtBoundsControlComponent::OnPointerEndGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtBoundsControlAffordanceInfo **pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	if (TryReleaseGrabPointer(**pAffordance))
	{
		OnManipulationEnded.Broadcast(this, **pAffordance, Grabbable);
	}
}

bool UUxtBoundsControlComponent::TryActivateGrabPointer(const FUxtBoundsControlAffordanceInfo &Affordance, const FUxtGrabPointerData &GrabPointer)
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

bool UUxtBoundsControlComponent::TryReleaseGrabPointer(const FUxtBoundsControlAffordanceInfo &Affordance)
{
	int numRemoved = ActiveAffordanceGrabPointers.RemoveAll(
		[&Affordance](const TPair<const FUxtBoundsControlAffordanceInfo*, FUxtGrabPointerData> &item)
		{
			return item.Key == &Affordance;
		});
	return numRemoved > 0;
}

FUxtGrabPointerData* UUxtBoundsControlComponent::FindGrabPointer(const FUxtBoundsControlAffordanceInfo& Affordance)
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

bool UUxtBoundsControlComponent::GetRelativeBoxTransform(const FBox &Box, const FBox &RelativeTo, FTransform &OutTransform)
{
	FVector extA = Box.GetExtent();
	FVector extB = RelativeTo.GetExtent();

	bool valid = !(FMath::IsNearlyZero(extB.X) || FMath::IsNearlyZero(extB.Y) || FMath::IsNearlyZero(extB.Z));
	FVector scale = valid ? extA / extB : FVector::OneVector;
	OutTransform = FTransform(FRotator::ZeroRotator, Box.GetCenter() - RelativeTo.GetCenter() * scale, scale);
	return valid;
}

