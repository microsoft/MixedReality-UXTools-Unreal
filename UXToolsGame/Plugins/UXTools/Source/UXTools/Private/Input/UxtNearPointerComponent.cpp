// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtNearPointerComponent.h"
#include "Input/UxtPointerFocus.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtTouchTarget.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

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

UObject* UUxtNearPointerComponent::GetDefaultGrabTarget() const
{
	return GrabFocus->GetDefaultTarget();
}

void UUxtNearPointerComponent::SetDefaultGrabTarget(UObject* NewDefaultTarget)
{
	GrabFocus->SetDefaultTarget(NewDefaultTarget);
}

UObject* UUxtNearPointerComponent::GetDefaultTouchTarget() const
{
	return TouchFocus->GetDefaultTarget();
}

void UUxtNearPointerComponent::SetDefaultTouchTarget(UObject* NewDefaultTarget)
{
	TouchFocus->SetDefaultTarget(NewDefaultTarget);
}
