// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtPointerFocus.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtTouchTarget.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"

namespace
{
	/**
	 * Used for checking on which side of a front face touchable's front face the pointer
	 * sphere is. This is important as BeginTouch can only be called if the pointer sphere
	 * was not behind in the previous tick and is now behind in this tick.
	 * 
	 * This function assumes that the given primitive has a box collider.
	 */
	bool IsBehindFrontFace(UPrimitiveComponent* Primitive, FVector PointerPosition, float Radius)
	{
		check(Primitive != nullptr);

		// Front face touchables should have use a box collider
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
	 * Used to determine whether if touch has ended with a front face touchable. A touch
	 * ends if:
	 * - The pointer sphere moves back in front of the front face of the touchable
	 * - The pointer spher moves left/right/up/down beyond the touchable primitive
	 *   extents
	 * - The perpendicular distance from the pointer sphere to the front face exceeds the
	 *   given depth.
	 *
	 * This function assumes that the given primitive has a box collider.
	 */
	bool IsFrontFaceTouchEnded(UPrimitiveComponent* Primitive, FVector PointerPosition, float Radius, float Depth)
	{
		check(Primitive != nullptr);

		// Front face touchables should have use a box collider
		check(Primitive->GetCollisionShape().IsBox());

		auto ComponentTransform = Primitive->GetComponentTransform().ToMatrixNoScale();

		FVector LocalPosition = ComponentTransform.InverseTransformPosition(PointerPosition);

		FVector Extents = Primitive->GetCollisionShape().GetExtent();

		FVector Min = -Extents;

		FVector Max = Extents;
		Max.X = -Max.X + Depth; // depth is measured from the front face

		FBox TouchableVolume(Min, Max);
		FSphere TouchSphere(LocalPosition, Radius);

		return !FMath::SphereAABBIntersection(TouchSphere, TouchableVolume);
	}
}
UUxtNearPointerComponent::UUxtNearPointerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick after physics so overlaps reflect the latest physics state.
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	GrabFocus = new FUxtGrabPointerFocus();
	TouchFocus = new FUxtTouchPointerFocus();
}

UUxtNearPointerComponent::~UUxtNearPointerComponent()
{
	delete GrabFocus;
	delete TouchFocus;
}

void UUxtNearPointerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GrabFocus->ClearFocus(this);
	TouchFocus->ClearFocus(this);

	Super::EndPlay(EndPlayReason);
}

void UUxtNearPointerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	const FTransform GrabTransform = GetGrabPointerTransform();
	const FTransform TouchTransform = GetTouchPointerTransform();

	// Don't update the focused target if locked
	if (!bFocusLocked)
	{
		const FVector ProximityCenter = GrabTransform.GetLocation();

		FCollisionQueryParams QueryParams(NAME_None, false);

		TArray<FOverlapResult> Overlaps;
		/*bool HasBlockingOverlap = */ GetWorld()->OverlapMultiByChannel(Overlaps, ProximityCenter, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(ProximityRadius), QueryParams);

		GrabFocus->SelectClosestTarget(this, GrabTransform, Overlaps);
		TouchFocus->SelectClosestTarget(this, TouchTransform, Overlaps);
	}
	
	// Update touch
	{
		FVector TouchPointerLocation = GetTouchPointerTransform().GetLocation();

		if (bIsTouching)
		{
			auto Target = TouchTargetWeak.Get();
			auto Primitive = TouchPrimitiveWeak.Get();
			
			check(Target != nullptr);
			check(Primitive != nullptr);

			bool endedTouching = false;

			switch (IUxtTouchTarget::Execute_GetTouchBehaviour(Target))
			{
			case EUxtTouchBehaviour::FrontFace:
				endedTouching = IsFrontFaceTouchEnded(Primitive, TouchPointerLocation, GetTouchPointerRadius(), TouchDepth);
				break;
			case EUxtTouchBehaviour::Volume:
				endedTouching = !Primitive->OverlapComponent(TouchPointerLocation, FQuat::Identity, FCollisionShape::MakeSphere(GetTouchPointerRadius()));
				break;
			}

			if (endedTouching) 
			{
				bIsTouching = false;
				TouchTargetWeak = nullptr;

				IUxtTouchTarget::Execute_OnEndTouch(Target, this);
			}
			else
			{
				FUxtPointerInteractionData Data;
				FTransform Transform = GetTouchPointerTransform();
				Data.Location = Transform.GetLocation();
				Data.Rotation = Transform.GetRotation();

				IUxtTouchTarget::Execute_OnUpdateTouch(Target, this, Data);
			}
		}
		else
		{
			FVector Start = PreviousTouchPointerLocation;
			FVector End = TouchPointerLocation;

			bool isBehind = bWasBehindFrontFace;
			if (auto FocusTarget = TouchFocus->GetFocusedPrimitive())
			{
				isBehind = IsBehindFrontFace(FocusTarget, End, GetTouchPointerRadius());
			}

			FHitResult HitResult;
			GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(GetTouchPointerRadius()));

			if (HitResult.GetActor())
			{
				if (auto HitTarget = TouchFocus->FindInterfaceComponent(HitResult.GetActor()))
				{
					bool startedTouching = false;

					switch (IUxtTouchTarget::Execute_GetTouchBehaviour(HitTarget))
					{
					case EUxtTouchBehaviour::FrontFace:
						startedTouching = !bWasBehindFrontFace && isBehind;
						break;
					case EUxtTouchBehaviour::Volume:
						startedTouching = true;
						break;
					}

					if (startedTouching)
					{
						bIsTouching = true;
						TouchTargetWeak = HitTarget;
						TouchPrimitiveWeak = HitResult.GetComponent();

						FUxtPointerInteractionData Data;
						FTransform Transform = GetTouchPointerTransform();
						Data.Location = Transform.GetLocation();
						Data.Rotation = Transform.GetRotation();

						IUxtTouchTarget::Execute_OnBeginTouch(HitTarget, this, Data);
					}
				}
			}

			bWasBehindFrontFace = isBehind;
		}

		PreviousTouchPointerLocation = TouchPointerLocation;
	}

	// Update focused targets

	GrabFocus->UpdateFocus(this, GrabTransform);
	GrabFocus->UpdateGrab(this, GrabTransform);

	TouchFocus->UpdateFocus(this, TouchTransform);

	// Update the grab state

	bool bHandIsGrabbing;
	if (UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(Hand, bHandIsGrabbing))
	{
		if (bHandIsGrabbing != GrabFocus->IsGrabbing())
		{
			if (bHandIsGrabbing)
			{
				GrabFocus->BeginGrab(this, GetGrabPointerTransform());
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
		TouchFocus->ClearFocus(this);
	}
}

EControllerHand UUxtNearPointerComponent::GetHand() const
{
	return Hand;
}

void UUxtNearPointerComponent::SetHand(EControllerHand NewHand)
{
	Hand = NewHand;
}

ECollisionChannel UUxtNearPointerComponent::GetTraceChannel() const
{
	return TraceChannel;
}

void UUxtNearPointerComponent::SetTraceChannel(ECollisionChannel NewTraceChannel)
{
	TraceChannel = NewTraceChannel;
}

float UUxtNearPointerComponent::GetProximityRadius() const
{
	return ProximityRadius;
}

void UUxtNearPointerComponent::SetProximityRadius(float Radius)
{
	this->ProximityRadius = Radius;
}

float UUxtNearPointerComponent::GetTouchRadius() const
{
	return TouchRadius;
}

void UUxtNearPointerComponent::SetTouchRadius(float Radius)
{
	this->TouchRadius = Radius;
}

float UUxtNearPointerComponent::GetGrabRadius() const
{
	return GrabRadius;
}

void UUxtNearPointerComponent::SetGrabRadius(float Radius)
{
	this->GrabRadius = Radius;
}

float UUxtNearPointerComponent::GetTouchDepth() const
{
	return TouchDepth;
}

void UUxtNearPointerComponent::SetTouchDepth(float depth)
{
	TouchDepth = depth;
}

UObject* UUxtNearPointerComponent::GetFocusedGrabTarget(FVector& OutClosestPointOnTarget) const
{
	OutClosestPointOnTarget = GrabFocus->GetClosestTargetPoint();
	return GrabFocus->GetFocusedTarget();
}

UObject* UUxtNearPointerComponent::GetFocusedTouchTarget(FVector& OutClosestPointOnTarget) const
{
	OutClosestPointOnTarget = TouchFocus->GetClosestTargetPoint();
	return TouchFocus->GetFocusedTarget();
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

bool UUxtNearPointerComponent::SetFocusedTouchTarget(UActorComponent* NewFocusedTarget, bool bEnableFocusLock)
{
	if (!bFocusLocked)
	{
		TouchFocus->SelectClosestPointOnTarget(this, GetTouchPointerTransform(), NewFocusedTarget);

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

bool UUxtNearPointerComponent::GetIsTouching() const
{
	return bIsTouching;
}

FTransform UUxtNearPointerComponent::GetGrabPointerTransform() const
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

FTransform UUxtNearPointerComponent::GetTouchPointerTransform() const
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

float UUxtNearPointerComponent::GetTouchPointerRadius() const
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
