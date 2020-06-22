// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFingerCursorComponent.h"
#include "UXTools.h"
#include "Input/UxtNearPointerComponent.h"
#include "GameFramework/Actor.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

namespace
{
	/**
	 * The cursor interpolates between two different transforms as it approaches the target.
	 * The first transform, which has a greater influence further away from the target, is 
	 * constructed as follows:
	 * - Location: (fingertip pos) + (tip radius) * (dir from knuckle to fingertip)
	 * - Rotation: (fingertip rot)
	 * 
	 * The second transform, Which has a greater influence closer to the target, is constructed 
	 * as follows:
	 * - Location: (fingertip pos) + (tip radius) * (dir from fingertip to point on target)
	 * - Rotation: (rot corresponding to dir from fingertip to point on target)
	 */
	FTransform GetCursorTransform(EControllerHand Hand, FVector PointOnTarget, float AlignWithSurfaceDistance)
	{
		bool foundValues = true;

		FQuat IndexTipOrientation;
		FVector IndexTipPosition;
		float IndexTipRadius;
		
		foundValues &= UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius);

		FQuat IndexKnuckleOrientation;
		FVector IndexKnucklePosition;
		float IndexKnuckleRadius;

		foundValues &= UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexProximal, IndexKnuckleOrientation, IndexKnucklePosition, IndexKnuckleRadius);

		if (!foundValues)
		{
			return FTransform::Identity;
		}

		auto FingerDir = (IndexTipPosition - IndexKnucklePosition);
		FingerDir.Normalize();

		auto ToTargetDir = PointOnTarget - IndexTipPosition;
		const auto DistanceToTarget = ToTargetDir.Size();
		ToTargetDir.Normalize();

		FVector Location;
		FQuat Rotation;

		if (DistanceToTarget < AlignWithSurfaceDistance)
		{
			float SlerpAmount = DistanceToTarget / AlignWithSurfaceDistance;

			FQuat FullRotation = FQuat::FindBetweenNormals(FingerDir, ToTargetDir);
			FVector Dir = FQuat::Slerp(FullRotation, FQuat::Identity, SlerpAmount) * FingerDir;

			Location = IndexTipPosition + Dir * IndexTipRadius;
			Rotation = FQuat::Slerp(ToTargetDir.ToOrientationQuat(), IndexTipOrientation, SlerpAmount);
		}
		else
		{
			Location = IndexTipPosition + FingerDir * IndexTipRadius;
			Rotation = IndexTipOrientation;
		}

		return FTransform(Rotation, Location);
	}
}

UUxtFingerCursorComponent::UUxtFingerCursorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	RingThickness = 0.3f;
	BorderThickness = 0.02f;

	// We want the ring to remain a constant thickness regardless of the radius
	bUseAbsoluteThickness = true;

	// Remain hidden until we see a valid poke target
	SetHiddenInGame(true);
}

void UUxtFingerCursorComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	UUxtNearPointerComponent* HandPointer = Owner->FindComponentByClass<UUxtNearPointerComponent>();
	HandPointerWeak = HandPointer;

	if (HandPointer)
	{
		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(HandPointer);
	}
	else
	{
		UE_LOG(UXTools, Error, TEXT("Could not find a near pointer in actor '%s'. Finger cursor won't work properly."), *Owner->GetName());
	}
}

void UUxtFingerCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtNearPointerComponent* HandPointer = HandPointerWeak.Get())
	{
		FVector PointOnTarget;
		FTransform PointerTransform;

		UObject* Target = HandPointer->GetFocusedPokeTarget(PointOnTarget);
		if (Target)
		{
			PointerTransform = HandPointer->GetPokePointerTransform();
		}
		else if (bShowOnGrabTargets)
		{
			Target = HandPointer->GetFocusedGrabTarget(PointOnTarget);

			if (Target)
			{
				PointerTransform = HandPointer->GetGrabPointerTransform();
			}
		}

		if (Target)
		{
			const float DistanceToTarget = FVector::Dist(PointOnTarget, PointerTransform.GetLocation());

			// Must use an epsilon to avoid unreliable rotations as we get closer to the target
			const float Epsilon = 0.000001;

			FTransform CursorTransform = GetCursorTransform(HandPointer->Hand, PointOnTarget, AlignWithSurfaceDistance);

			if (DistanceToTarget > Epsilon)
			{
				SetWorldTransform(CursorTransform);
			}
			else
			{
				SetWorldLocation(CursorTransform.GetLocation());
			}

			// Scale radius with the distance to the target
			float Alpha = DistanceToTarget / MaxDistanceToTarget;
			float NewRadius = FMath::Lerp(MinRadius, MaxRadius, Alpha);
			SetRadius(NewRadius);

			if (bHiddenInGame)
			{
				SetHiddenInGame(false);
			}
		}
		else if(!bHiddenInGame)
		{
			// Hide mesh when the pointer has no target
			SetHiddenInGame(true);
		}
	}
}
