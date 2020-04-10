// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtPointerFocus.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeTarget.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"

namespace
{
	/**
	 * Used for checking on which side of a front face pokable's front face the pointer
	 * sphere is. This is important as BeginPoke can only be called if the pointer sphere
	 * was not behind in the previous tick and is now behind in this tick.
	 * 
	 * This function assumes that the given primitive has a box collider.
	 */
	bool IsBehindFrontFace(UPrimitiveComponent* Primitive, FVector PointerPosition, float Radius)
	{
		check(Primitive != nullptr);

		// Front face pokables should have use a box collider
		check(Primitive->GetCollisionShape().IsBox());

		auto ComponentTransform = Primitive->GetComponentTransform().ToMatrixNoScale();
		
		FVector LocalPosition = ComponentTransform.InverseTransformPosition(PointerPosition);

		FVector Extents = Primitive->GetCollisionShape().GetExtent();

		if (LocalPosition.X + Radius > -Extents.X)
		{
			return true;
		}

		return false;
	}

	/** 
	 * Used to determine whether if poke has ended with a front face pokable. A poke
	 * ends if:
	 * - The pointer sphere moves back in front of the front face of the pokable
	 * - The pointer spher moves left/right/up/down beyond the pokable primitive
	 *   extents
	 * - The perpendicular distance from the pointer sphere to the front face exceeds the
	 *   given depth.
	 *
	 * This function assumes that the given primitive has a box collider.
	 */
	bool IsFrontFacePokeEnded(UPrimitiveComponent* Primitive, FVector PointerPosition, float Radius, float Depth)
	{
		check(Primitive != nullptr);

		// Front face pokables should have use a box collider
		check(Primitive->GetCollisionShape().IsBox());

		auto ComponentTransform = Primitive->GetComponentTransform().ToMatrixNoScale();

		FVector LocalPosition = ComponentTransform.InverseTransformPosition(PointerPosition);

		FVector Extents = Primitive->GetCollisionShape().GetExtent();

		FVector Min = -Extents;

		FVector Max = Extents;
		Max.X = -Max.X + Depth; // depth is measured from the front face

		FBox PokableVolume(Min, Max);
		FSphere PokeSphere(LocalPosition, Radius);

		return !FMath::SphereAABBIntersection(PokeSphere, PokableVolume);
	}
}
UUxtNearPointerComponent::UUxtNearPointerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick after physics so overlaps reflect the latest physics state.
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	GrabFocus = new FUxtGrabPointerFocus();
	PokeFocus = new FUxtPokePointerFocus();
}

UUxtNearPointerComponent::~UUxtNearPointerComponent()
{
	delete GrabFocus;
	delete PokeFocus;
}

void UUxtNearPointerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GrabFocus->ClearFocus(this);
	PokeFocus->ClearFocus(this);

	Super::EndPlay(EndPlayReason);
}

static FTransform CalcGrabPointerTransform(EControllerHand Hand)
{
	FQuat IndexTipOrientation, ThumbTipOrientation;
	FVector IndexTipPosition, ThumbTipPosition;
	float IndexTipRadius, ThumbTipRadius;
	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius)
		&& UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::ThumbTip, ThumbTipOrientation, ThumbTipPosition, ThumbTipRadius))
	{
		// Use the midway point between the thumb and index finger tips for grab
		const float LerpFactor = 0.5f;
		return FTransform(FMath::Lerp(IndexTipOrientation, ThumbTipOrientation, LerpFactor), FMath::Lerp(IndexTipPosition, ThumbTipPosition, LerpFactor));
	}
	return FTransform::Identity;
}

static FTransform CalcPokePointerTransform(EControllerHand Hand)
{
	FQuat IndexTipOrientation;
	FVector IndexTipPosition;
	float IndexTipRadius;
	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius))
	{
		return FTransform(IndexTipOrientation, IndexTipPosition);
	}
	return FTransform::Identity;
}

void UUxtNearPointerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Update cached transforms
	GrabPointerTransform = CalcGrabPointerTransform(Hand);
	PokePointerTransform = CalcPokePointerTransform(Hand);

	// Unlock focus if targets have been removed,
	// e.g. if target actors are destroyed while focus locked.
	if (bFocusLocked)
	{
		if (!GrabFocus->GetFocusedTarget() && !PokeFocus->GetFocusedTarget())
		{
			bFocusLocked = false;
		}
	}

	// Don't update the focused target if locked
	if (!bFocusLocked)
	{
		const FVector ProximityCenter = GrabPointerTransform.GetLocation();

		FCollisionQueryParams QueryParams(NAME_None, false);

		TArray<FOverlapResult> Overlaps;
		/*bool HasBlockingOverlap = */ GetWorld()->OverlapMultiByChannel(Overlaps, ProximityCenter, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(ProximityRadius), QueryParams);

		GrabFocus->SelectClosestTarget(this, GrabPointerTransform, Overlaps);
		PokeFocus->SelectClosestTarget(this, PokePointerTransform, Overlaps);
	}
	
	// Update poke
	{
		FVector PokePointerLocation = GetPokePointerTransform().GetLocation();
		UActorComponent* Target = Cast<UActorComponent>(PokeFocus->GetFocusedTarget());
		UPrimitiveComponent* Primitive = PokeFocus->GetFocusedPrimitive();


		if (bIsPoking)
		{
			if (Primitive && Target)
			{
				bool endedPoking = false;

				switch (IUxtPokeTarget::Execute_GetPokeBehaviour(Target))
				{
				case EUxtPokeBehaviour::FrontFace:
					endedPoking = IsFrontFacePokeEnded(Primitive, PokePointerLocation, GetPokePointerRadius(), PokeDepth);
					break;
				case EUxtPokeBehaviour::Volume:
					endedPoking = !Primitive->OverlapComponent(PokePointerLocation, FQuat::Identity, FCollisionShape::MakeSphere(GetPokePointerRadius()));
					break;
				}

				if (endedPoking)
				{
					bIsPoking = false;
					IUxtPokeTarget::Execute_OnEndPoke(Target, this);
				}
				else
				{
					IUxtPokeTarget::Execute_OnUpdatePoke(Target, this);
				}
			}
			else
			{
				bIsPoking = false;
				bFocusLocked = false;
			}
		}
		else if (Target)
		{
			FVector Start = PreviousPokePointerLocation;
			FVector End = PokePointerLocation;

			bool isBehind = bWasBehindFrontFace;
			if (Primitive)
			{
				isBehind = IsBehindFrontFace(Primitive, End, GetPokePointerRadius());
			}

			FHitResult HitResult;
			GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(GetPokePointerRadius()));

			if (HitResult.GetComponent() == Primitive)
			{
				bool startedPoking = false;

				switch (IUxtPokeTarget::Execute_GetPokeBehaviour(Target))
				{
				case EUxtPokeBehaviour::FrontFace:
					startedPoking = !bWasBehindFrontFace && isBehind;
					break;
				case EUxtPokeBehaviour::Volume:
					startedPoking = true;
					break;
				}

				if (startedPoking)
				{
					bIsPoking = true;
					IUxtPokeTarget::Execute_OnBeginPoke(Target, this);
				}
			}

			bWasBehindFrontFace = isBehind;
		}

		PreviousPokePointerLocation = PokePointerLocation;
	}

	// Update focused targets

	GrabFocus->UpdateFocus(this);
	GrabFocus->UpdateGrab(this);

	PokeFocus->UpdateFocus(this);

	// Update the grab state

	bool bHandIsGrabbing;
	if (UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(Hand, bHandIsGrabbing))
	{
		if (bHandIsGrabbing != GrabFocus->IsGrabbing())
		{
			if (bHandIsGrabbing)
			{
				GrabFocus->BeginGrab(this);
			}
			else
			{
				GrabFocus->EndGrab(this);
			}
		}
	}
}

void UUxtNearPointerComponent::SetActive(bool bNewActive, bool bReset)
{
	bool bOldActive = IsActive();
	Super::SetActive(bNewActive, bReset);

	if (bOldActive && !bNewActive)
	{
		GrabFocus->ClearFocus(this);
		PokeFocus->ClearFocus(this);
	}
}

UObject* UUxtNearPointerComponent::GetFocusedGrabTarget(FVector& OutClosestPointOnTarget) const
{
	OutClosestPointOnTarget = GrabFocus->GetClosestTargetPoint();
	return GrabFocus->GetFocusedTarget();
}

UObject* UUxtNearPointerComponent::GetFocusedPokeTarget(FVector& OutClosestPointOnTarget) const
{
	OutClosestPointOnTarget = PokeFocus->GetClosestTargetPoint();
	return PokeFocus->GetFocusedTarget();
}

bool UUxtNearPointerComponent::SetFocusedGrabTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		GrabFocus->SelectClosestPointOnTarget(this, GetGrabPointerTransform(), NewFocusedTarget);

		bFocusLocked = (NewFocusedTarget != nullptr && bEnableFocusLock);

		return true;
	}
	return false;
}

bool UUxtNearPointerComponent::SetFocusedPokeTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		PokeFocus->SelectClosestPointOnTarget(this, GetPokePointerTransform(), NewFocusedTarget);

		bFocusLocked = (NewFocusedTarget != nullptr && bEnableFocusLock);

		return true;
	}
	return false;
}

bool UUxtNearPointerComponent::GetFocusLocked() const
{
	return bFocusLocked;
}

void UUxtNearPointerComponent::SetFocusLocked(bool Value)
{
	bFocusLocked = Value;
}

bool UUxtNearPointerComponent::IsGrabbing() const
{
	return GrabFocus->IsGrabbing();
}

bool UUxtNearPointerComponent::GetIsPoking() const
{
	return bIsPoking;
}

FTransform UUxtNearPointerComponent::GetGrabPointerTransform() const
{
	return GrabPointerTransform;
}

FTransform UUxtNearPointerComponent::GetPokePointerTransform() const
{
	return PokePointerTransform;
}

float UUxtNearPointerComponent::GetPokePointerRadius() const
{
	FQuat IndexTipOrientation;
	FVector IndexTipPosition;
	float IndexTipRadius;
	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius))
	{
		return IndexTipRadius;
	}
	return 0;
}
