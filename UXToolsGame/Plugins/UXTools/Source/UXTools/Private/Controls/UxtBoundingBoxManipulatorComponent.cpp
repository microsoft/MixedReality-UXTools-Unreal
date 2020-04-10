// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBoundingBoxManipulatorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"


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

FTransform FUxtBoundingBoxAffordanceInfo::GetWorldTransform(const FBox &Bounds, const FTransform &RootTransform) const
{
	FVector location = Bounds.GetCenter() + Bounds.GetExtent() * BoundsLocation;
	FRotator rotation = BoundsRotation;
	FVector scale = FVector::OneVector;

	return FTransform(RootTransform.TransformRotation(FQuat(rotation)), RootTransform.TransformPosition(location), scale);
}


UUxtBoundingBoxManipulatorComponent::UUxtBoundingBoxManipulatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
}

const TArray<FUxtBoundingBoxAffordanceInfo>& UUxtBoundingBoxManipulatorComponent::GetCustomAffordances() const
{
	return CustomAffordances;
}

TSubclassOf<class AActor> UUxtBoundingBoxManipulatorComponent::GetCenterAffordanceClass() const
{
	return CenterAffordanceClass;
}

TSubclassOf<class AActor> UUxtBoundingBoxManipulatorComponent::GetFaceAffordanceClass() const
{
	return FaceAffordanceClass;
}

TSubclassOf<class AActor> UUxtBoundingBoxManipulatorComponent::GetEdgeAffordanceClass() const
{
	return EdgeAffordanceClass;
}

TSubclassOf<class AActor> UUxtBoundingBoxManipulatorComponent::GetCornerAffordanceClass() const
{
	return CornerAffordanceClass;
}

bool UUxtBoundingBoxManipulatorComponent::UseCustomAffordances() const
{
	return bUseCustomAffordances;
}

EUxtBoundingBoxManipulatorPreset UUxtBoundingBoxManipulatorComponent::GetPreset() const
{
	return Preset;
}

bool UUxtBoundingBoxManipulatorComponent::GetInitBoundsFromActor() const
{
	return bInitBoundsFromActor;
}

const TArray<FUxtBoundingBoxAffordanceInfo>& UUxtBoundingBoxManipulatorComponent::GetUsedAffordances() const
{
	if (bUseCustomAffordances)
	{
		return CustomAffordances;
	}
	else
	{
		return FUxtBoundingBoxPresetUtils::GetPresetAffordances(Preset);
	}
}

TSubclassOf<class AActor> UUxtBoundingBoxManipulatorComponent::GetAffordanceKindActorClass(EUxtBoundingBoxAffordanceKind Kind) const
{
	switch (Kind)
	{
	case EUxtBoundingBoxAffordanceKind::Center:	return CenterAffordanceClass;
	case EUxtBoundingBoxAffordanceKind::Face:		return FaceAffordanceClass;
	case EUxtBoundingBoxAffordanceKind::Edge:		return EdgeAffordanceClass;
	case EUxtBoundingBoxAffordanceKind::Corner:	return CornerAffordanceClass;
	}

	return nullptr;
}

const FBox& UUxtBoundingBoxManipulatorComponent::GetBounds() const
{
	return Bounds;
}

void UUxtBoundingBoxManipulatorComponent::ComputeBoundsFromComponents()
{
	Bounds = CalculateNestedActorBoundsInLocalSpace(GetOwner(), true);

	UpdateAffordanceTransforms();
}

void UUxtBoundingBoxManipulatorComponent::UpdateAffordanceTransforms()
{
	for (const auto &item : ActorAffordanceMap)
	{
		FTransform affordanceTransform = item.Value->GetWorldTransform(Bounds, GetOwner()->GetActorTransform());
		item.Key->SetActorTransform(affordanceTransform);
	}
}

void UUxtBoundingBoxManipulatorComponent::BeginPlay()
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

	// Create affordances
	const auto &usedAffordances = GetUsedAffordances();
	for (const FUxtBoundingBoxAffordanceInfo &affordance : usedAffordances)
	{
		auto affordanceClass = (affordance.ActorClass != nullptr ? affordance.ActorClass : GetAffordanceKindActorClass(affordance.Kind));
		if (IsValid(affordanceClass))
		{
			AActor *affordanceActor = GetWorld()->SpawnActor<AActor>(affordanceClass);

			if (affordanceActor != nullptr)
			{
				ActorAffordanceMap.Add(affordanceActor, &affordance);

				UUxtGrabTargetComponent *grabbable = affordanceActor->FindComponentByClass<UUxtGrabTargetComponent>();
				if (grabbable != nullptr)
				{
					grabbable->OnBeginGrab.AddDynamic(this, &UUxtBoundingBoxManipulatorComponent::OnPointerBeginGrab);
					grabbable->OnUpdateGrab.AddDynamic(this, &UUxtBoundingBoxManipulatorComponent::OnPointerUpdateGrab);
					grabbable->OnEndGrab.AddDynamic(this, &UUxtBoundingBoxManipulatorComponent::OnPointerEndGrab);
				}
			}
		}
	}

	UpdateAffordanceTransforms();
}

void UUxtBoundingBoxManipulatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Drop active grab pointers.
	ActiveAffordanceGrabPointers.Empty();

	// Destroy affordances
	for (const auto &item : ActorAffordanceMap)
	{
		GetWorld()->DestroyActor(item.Key);
	}
	ActorAffordanceMap.Empty();

	Super::EndPlay(EndPlayReason);
}

void UUxtBoundingBoxManipulatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveAffordanceGrabPointers.Num() > 0)
	{
		// Get the active affordance data
		// Only one grab at a time supported for now
		check(ActiveAffordanceGrabPointers.Num() == 1);
		const FUxtBoundingBoxAffordanceInfo &affordance = *ActiveAffordanceGrabPointers[0].Key;
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
}

void UUxtBoundingBoxManipulatorComponent::ComputeModifiedBounds(const FUxtBoundingBoxAffordanceInfo &Affordance, const FUxtGrabPointerData &GrabPointer, FBox &OutBounds, FQuat &OutDeltaRotation) const
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
	case EUxtBoundingBoxAffordanceAction::Resize:
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

	case EUxtBoundingBoxAffordanceAction::Translate:
	{
		FVector localDelta = localTarget - localGrabPoint;
		FVector constrainedDelta = affordanceConstraint.TransformVector(localDelta);

		// All sides moving together
		OutBounds.Min += constrainedDelta;
		OutBounds.Max += constrainedDelta;
		break;
	}

	case EUxtBoundingBoxAffordanceAction::Scale:
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

	case EUxtBoundingBoxAffordanceAction::Rotate:
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

void UUxtBoundingBoxManipulatorComponent::OnPointerBeginGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtBoundingBoxAffordanceInfo **pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	FUxtGrabPointerData bboxGrabPointer;
	bboxGrabPointer.NearPointer = GrabPointer.NearPointer;
	bboxGrabPointer.GrabPointTransform = GrabPointer.GrabPointTransform;
	bboxGrabPointer.StartTime = GrabPointer.StartTime;
	// Transform into the bbox actor space
	FTransform relTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
	bboxGrabPointer.LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(relTransform, GrabPointer);

	TryActivateGrabPointer(**pAffordance, bboxGrabPointer);
}

void UUxtBoundingBoxManipulatorComponent::OnPointerUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtBoundingBoxAffordanceInfo** pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	FUxtGrabPointerData* pBBoxGrabPointer = FindGrabPointer(**pAffordance);
	if (ensure(pBBoxGrabPointer))
	{
		pBBoxGrabPointer->NearPointer = GrabPointer.NearPointer;
		pBBoxGrabPointer->GrabPointTransform = GrabPointer.GrabPointTransform;
		pBBoxGrabPointer->StartTime = GrabPointer.StartTime;
		// Transform into the bbox actor space
		FTransform relTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
		pBBoxGrabPointer->LocalGrabPoint = UUxtGrabPointerDataFunctionLibrary::GetGrabTransform(relTransform, GrabPointer);
	}
}

void UUxtBoundingBoxManipulatorComponent::OnPointerEndGrab(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer)
{
	const FUxtBoundingBoxAffordanceInfo **pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	TryReleaseGrabPointer(**pAffordance);
}

bool UUxtBoundingBoxManipulatorComponent::TryActivateGrabPointer(const FUxtBoundingBoxAffordanceInfo &Affordance, const FUxtGrabPointerData &GrabPointer)
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

bool UUxtBoundingBoxManipulatorComponent::TryReleaseGrabPointer(const FUxtBoundingBoxAffordanceInfo &Affordance)
{
	int numRemoved = ActiveAffordanceGrabPointers.RemoveAll(
		[&Affordance](const TPair<const FUxtBoundingBoxAffordanceInfo*, FUxtGrabPointerData> &item)
		{
			return item.Key == &Affordance;
		});
	return numRemoved > 0;
}

FUxtGrabPointerData* UUxtBoundingBoxManipulatorComponent::FindGrabPointer(const FUxtBoundingBoxAffordanceInfo& Affordance)
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

bool UUxtBoundingBoxManipulatorComponent::GetRelativeBoxTransform(const FBox &Box, const FBox &RelativeTo, FTransform &OutTransform)
{
	FVector extA = Box.GetExtent();
	FVector extB = RelativeTo.GetExtent();

	bool valid = !(FMath::IsNearlyZero(extB.X) || FMath::IsNearlyZero(extB.Y) || FMath::IsNearlyZero(extB.Z));
	FVector scale = valid ? extA / extB : FVector::OneVector;
	OutTransform = FTransform(FRotator::ZeroRotator, Box.GetCenter() - RelativeTo.GetCenter() * scale, scale);
	return valid;
}

