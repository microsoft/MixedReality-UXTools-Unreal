// Fill out your copyright notice in the Description page of Project Settings.

#include "BoundingBoxManipulatorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "MixedRealityMathUtilsFunctionLibrary.h"


FTransform FBoundingBoxAffordanceInfo::GetWorldTransform(const FBox &Bounds, const FTransform &RootTransform) const
{
	FVector location = Bounds.GetCenter() + Bounds.GetExtent() * BoundsLocation;
	FRotator rotation = BoundsRotation;
	FVector scale = FVector::OneVector;

	return FTransform(RootTransform.TransformRotation(FQuat(rotation)), RootTransform.TransformPosition(location), scale);
}


UBoundingBoxManipulatorComponent::UBoundingBoxManipulatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;
}

const TArray<FBoundingBoxAffordanceInfo>& UBoundingBoxManipulatorComponent::GetCustomAffordances() const
{
	return CustomAffordances;
}

TSubclassOf<class AActor> UBoundingBoxManipulatorComponent::GetCenterAffordanceClass() const
{
	return CenterAffordanceClass;
}

TSubclassOf<class AActor> UBoundingBoxManipulatorComponent::GetFaceAffordanceClass() const
{
	return FaceAffordanceClass;
}

TSubclassOf<class AActor> UBoundingBoxManipulatorComponent::GetEdgeAffordanceClass() const
{
	return EdgeAffordanceClass;
}

TSubclassOf<class AActor> UBoundingBoxManipulatorComponent::GetCornerAffordanceClass() const
{
	return CornerAffordanceClass;
}

bool UBoundingBoxManipulatorComponent::UseCustomAffordances() const
{
	return bUseCustomAffordances;
}

EBoundingBoxManipulatorPreset UBoundingBoxManipulatorComponent::GetPreset() const
{
	return Preset;
}

bool UBoundingBoxManipulatorComponent::GetInitBoundsFromActor() const
{
	return bInitBoundsFromActor;
}

const TArray<FBoundingBoxAffordanceInfo>& UBoundingBoxManipulatorComponent::GetUsedAffordances() const
{
	if (bUseCustomAffordances)
	{
		return CustomAffordances;
	}
	else
	{
		return BoundingBoxPresetUtils::GetPresetAffordances(Preset);
	}
}

TSubclassOf<class AActor> UBoundingBoxManipulatorComponent::GetAffordanceKindActorClass(EBoundingBoxAffordanceKind Kind) const
{
	switch (Kind)
	{
	case EBoundingBoxAffordanceKind::Center:	return CenterAffordanceClass;
	case EBoundingBoxAffordanceKind::Face:		return FaceAffordanceClass;
	case EBoundingBoxAffordanceKind::Edge:		return EdgeAffordanceClass;
	case EBoundingBoxAffordanceKind::Corner:	return CornerAffordanceClass;
	}

	return nullptr;
}

const FBox& UBoundingBoxManipulatorComponent::GetBounds() const
{
	return Bounds;
}

void UBoundingBoxManipulatorComponent::ComputeBoundsFromComponents()
{
	Bounds = GetOwner()->CalculateComponentsBoundingBoxInLocalSpace(true);
}

void UBoundingBoxManipulatorComponent::UpdateAffordanceTransforms()
{
	for (const auto &item : ActorAffordanceMap)
	{
		FTransform affordanceTransform = item.Value->GetWorldTransform(Bounds, GetOwner()->GetActorTransform());
		item.Key->SetActorTransform(affordanceTransform);
	}
}

void UBoundingBoxManipulatorComponent::BeginPlay()
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
	for (const FBoundingBoxAffordanceInfo &affordance : usedAffordances)
	{
		auto affordanceClass = (affordance.ActorClass != nullptr ? affordance.ActorClass : GetAffordanceKindActorClass(affordance.Kind));
		if (IsValid(affordanceClass))
		{
			AActor *affordanceActor = GetWorld()->SpawnActor<AActor>(affordanceClass);

			if (affordanceActor != nullptr)
			{
				ActorAffordanceMap.Add(affordanceActor, &affordance);

				UGrabbableComponent *grabbable = affordanceActor->FindComponentByClass<UGrabbableComponent>();
				if (grabbable != nullptr)
				{
					grabbable->OnBeginGrab.AddDynamic(this, &UBoundingBoxManipulatorComponent::OnPointerBeginGrab);
					grabbable->OnEndGrab.AddDynamic(this, &UBoundingBoxManipulatorComponent::OnPointerEndGrab);
				}
			}
		}
	}

	UpdateAffordanceTransforms();
}

void UBoundingBoxManipulatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void UBoundingBoxManipulatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveAffordanceGrabPointers.Num() > 0)
	{
		// Get the active affordance data
		// Only one grab at a time supported for now
		check(ActiveAffordanceGrabPointers.Num() == 1);
		const FBoundingBoxAffordanceInfo &affordance = *ActiveAffordanceGrabPointers[0].Key;
		const FGrabPointerData &grabPointer = ActiveAffordanceGrabPointers[0].Value;

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
			newTransform = UMixedRealityMathUtilsFunctionLibrary::RotateAboutPivotPoint(newTransform, deltaRotation.Rotator(), pivot);
			GetOwner()->SetActorTransform(newTransform);
		}

		UpdateAffordanceTransforms();
	}
}

void UBoundingBoxManipulatorComponent::ComputeModifiedBounds(const FBoundingBoxAffordanceInfo &Affordance, const FGrabPointerData &GrabPointer, FBox &OutBounds, FQuat &OutDeltaRotation) const
{
	//
	// Look up settings for the affordance

	const FVector affordanceLoc = Affordance.BoundsLocation;
	const FMatrix affordanceConstraint = Affordance.ConstraintMatrix;

	//
	// Compute grab pointer movement

	const FVector localGrabPoint = GrabPointer.LocalGrabPoint.GetTranslation();
	const FVector grabPoint = InitialTransform.TransformPosition(localGrabPoint);

	const FVector target = UGrabPointerDataFunctionLibrary::GetTargetLocation(GrabPointer);
	const FVector localTarget = InitialTransform.InverseTransformPosition(target);

	//
	// Compute modified bounding box

	OutBounds = InitialBounds;
	OutDeltaRotation = FQuat::Identity;

	switch (Affordance.Action)
	{
	case EBoundingBoxAffordanceAction::Resize:
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

	case EBoundingBoxAffordanceAction::Translate:
	{
		FVector localDelta = localTarget - localGrabPoint;
		FVector constrainedDelta = affordanceConstraint.TransformVector(localDelta);

		// All sides moving together
		OutBounds.Min += constrainedDelta;
		OutBounds.Max += constrainedDelta;
		break;
	}

	case EBoundingBoxAffordanceAction::Scale:
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

	case EBoundingBoxAffordanceAction::Rotate:
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

void UBoundingBoxManipulatorComponent::OnPointerBeginGrab(UGrabbableComponent *Grabbable, FGrabPointerData GrabPointer)
{
	const FBoundingBoxAffordanceInfo **pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	FGrabPointerData bboxGrabPointer;
	bboxGrabPointer.Pointer = GrabPointer.Pointer;
	bboxGrabPointer.StartTime = GrabPointer.StartTime;
	// Transform into the bbox actor space
	FTransform relTransform = Grabbable->GetComponentTransform().GetRelativeTransform(GetOwner()->GetActorTransform());
	bboxGrabPointer.LocalGrabPoint = UGrabPointerDataFunctionLibrary::GetGrabTransform(relTransform, GrabPointer);

	TryActivateGrabPointer(**pAffordance, bboxGrabPointer);
}

void UBoundingBoxManipulatorComponent::OnPointerEndGrab(UGrabbableComponent *Grabbable, FGrabPointerData GrabPointer)
{
	const FBoundingBoxAffordanceInfo **pAffordance = ActorAffordanceMap.Find(Grabbable->GetOwner());
	check(pAffordance != nullptr);

	TryReleaseGrabPointer(**pAffordance);
}

bool UBoundingBoxManipulatorComponent::TryActivateGrabPointer(const FBoundingBoxAffordanceInfo &Affordance, const FGrabPointerData &GrabPointer)
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

bool UBoundingBoxManipulatorComponent::TryReleaseGrabPointer(const FBoundingBoxAffordanceInfo &Affordance)
{
	int numRemoved = ActiveAffordanceGrabPointers.RemoveAll(
		[&Affordance](const TPair<const FBoundingBoxAffordanceInfo*, FGrabPointerData> &item)
		{
			return item.Key == &Affordance;
		});
	return numRemoved > 0;
}

bool UBoundingBoxManipulatorComponent::GetRelativeBoxTransform(const FBox &Box, const FBox &RelativeTo, FTransform &OutTransform)
{
	FVector extA = Box.GetExtent();
	FVector extB = RelativeTo.GetExtent();

	bool valid = !(FMath::IsNearlyZero(extB.X) || FMath::IsNearlyZero(extB.Y) || FMath::IsNearlyZero(extB.Z));
	FVector scale = valid ? extA / extB : FVector::OneVector;
	OutTransform = FTransform(FRotator::ZeroRotator, Box.GetCenter() - RelativeTo.GetCenter() * scale, scale);
	return valid;
}
