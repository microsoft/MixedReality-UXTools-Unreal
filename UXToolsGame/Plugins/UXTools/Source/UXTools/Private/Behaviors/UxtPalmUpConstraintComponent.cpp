// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Behaviors/UxtPalmUpConstraintComponent.h"

#include "DrawDebugHelpers.h"
#include "EyeTrackerFunctionLibrary.h"

#include "Engine/World.h"
#include "HandTracking/IUxtHandTracker.h"
#include "Utils/UxtFunctionLibrary.h"

namespace
{
	bool GetActivationPoint(EControllerHand Hand, EUxtHandConstraintZone Zone, FVector& OutActivationPoint)
	{
		EHandKeypoint ReferenceJoint1 = EHandKeypoint::Palm;
		EHandKeypoint ReferenceJoint2 = EHandKeypoint::Palm;

		switch (Zone)
		{
		case EUxtHandConstraintZone::UlnarSide:
			ReferenceJoint1 = EHandKeypoint::LittleMetacarpal;
			ReferenceJoint2 = EHandKeypoint::LittleMetacarpal;
			break;

		case EUxtHandConstraintZone::RadialSide:
			ReferenceJoint1 = EHandKeypoint::IndexMetacarpal;
			ReferenceJoint2 = EHandKeypoint::ThumbProximal;
			break;

		case EUxtHandConstraintZone::AboveFingerTips:
			ReferenceJoint1 = EHandKeypoint::MiddleTip;
			ReferenceJoint2 = EHandKeypoint::RingTip;
			break;

		case EUxtHandConstraintZone::BelowWrist:
			ReferenceJoint1 = EHandKeypoint::Wrist;
			ReferenceJoint2 = EHandKeypoint::Wrist;
			break;

		default:
			checkNoEntry();
		}

		FVector ReferenceJointLocation1;
		FVector ReferenceJointLocation2;
		FQuat Orientation;
		float Radius;
		if (!IUxtHandTracker::Get().GetJointState(Hand, ReferenceJoint1, Orientation, ReferenceJointLocation1, Radius) ||
			!IUxtHandTracker::Get().GetJointState(Hand, ReferenceJoint2, Orientation, ReferenceJointLocation2, Radius))
		{
			return false;
		}

		OutActivationPoint = FMath::Lerp(ReferenceJointLocation1, ReferenceJointLocation2, 0.5f);
		return true;
	}

	bool GetHandPlaneAndActivationPoint(EControllerHand Hand, EUxtHandConstraintZone Zone, FPlane& OutHandPlane, FVector& OutActivationPoint)
	{
		FVector WristPosition;
		FVector IndexPosition;
		FVector LittlePosition;
		FQuat Orientation;
		float Radius;

		if (!IUxtHandTracker::Get().GetJointState(Hand, EHandKeypoint::Wrist, Orientation, WristPosition, Radius) ||
			!IUxtHandTracker::Get().GetJointState(Hand, EHandKeypoint::IndexMetacarpal, Orientation, IndexPosition, Radius) ||
			!IUxtHandTracker::Get().GetJointState(Hand, EHandKeypoint::LittleMetacarpal, Orientation, LittlePosition, Radius))
		{
			return false;
		}

		FVector ActivationPoint;
		if (!GetActivationPoint(Hand, Zone, ActivationPoint))
		{
			return false;
		}

		OutHandPlane = FPlane(WristPosition, IndexPosition, LittlePosition);
		OutActivationPoint = FVector::PointPlaneProject(ActivationPoint, OutHandPlane);
		return true;
	}
} // namespace

bool UUxtPalmUpConstraintComponent::IsHandUsableForConstraint(EControllerHand NewHand)
{
	const FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());

	FQuat PalmRotation;
	FVector PalmLocation;
	float PalmRadius;
	if (!IUxtHandTracker::Get().GetJointState(NewHand, EHandKeypoint::Palm, PalmRotation, PalmLocation, PalmRadius))
	{
		return false;
	}
	// Note: Palm Z is normal to the back of the hand, not the inside
	const FVector PalmUpVector = PalmRotation.GetUpVector();

	if (!IsPalmUp(HeadPose, PalmLocation, PalmUpVector))
	{
		bGazeTriggered = false;
		return false;
	}

	if (bRequireFlatHand && !IsHandFlat(NewHand, PalmLocation, PalmUpVector))
	{
		bGazeTriggered = false;
		return false;
	}

	if (bRequireGaze && !bGazeTriggered)
	{
		bGazeTriggered = HasEyeGaze(NewHand, HeadPose, PalmLocation);
		if (!bGazeTriggered)
		{
			return false;
		}
	}

	return true;
}

bool UUxtPalmUpConstraintComponent::IsPalmUp(const FTransform& HeadPose, const FVector& PalmLocation, const FVector& PalmUpVector) const
{
	// Test palm normal against camera view angle
	FVector LookAtVector = PalmLocation - HeadPose.GetLocation();
	LookAtVector.Normalize();

	const float CosAngle = FVector::DotProduct(LookAtVector, PalmUpVector);
	const float MinCosAngle = FMath::Cos(FMath::DegreesToRadians(MaxPalmAngle));

	// Accept the hand if palm angle is within the cone limit
	return CosAngle >= MinCosAngle;
}

bool UUxtPalmUpConstraintComponent::IsHandFlat(EControllerHand NewHand, const FVector& PalmLocation, const FVector& PalmUpVector) const
{
	// Test Palm-Index-Ring triangle against palm for measuring flatness
	FQuat IndexRotation, RingRotation;
	FVector IndexLocation, RingLocation;
	float IndexRadius, RingRadius;
	if (!IUxtHandTracker::Get().GetJointState(NewHand, EHandKeypoint::IndexTip, IndexRotation, IndexLocation, IndexRadius) ||
		!IUxtHandTracker::Get().GetJointState(NewHand, EHandKeypoint::RingTip, RingRotation, RingLocation, RingRadius))
	{
		return false;
	}

	FVector FingerNormal = FVector::CrossProduct(RingLocation - PalmLocation, IndexLocation - PalmLocation).GetSafeNormal();
	if (NewHand != EControllerHand::Left)
	{
		FingerNormal = -FingerNormal;
	}

	const float CosFlatAngle = FVector::DotProduct(FingerNormal, PalmUpVector);
	const float MinCosFlagAngle = FMath::Cos(FMath::DegreesToRadians(MaxFlatHandAngle));

	// Accept the hand if finger angle is within the cone limit
	return CosFlatAngle >= MinCosFlagAngle;
}

bool UUxtPalmUpConstraintComponent::HasEyeGaze(EControllerHand NewHand, const FTransform& HeadPose, const FVector& PalmLocation) const
{
	// Test the eye / head gaze location against the activation zone
	FEyeTrackerGazeData GazeData;
	const bool bUsedEyeGaze = UEyeTrackerFunctionLibrary::GetGazeData(GazeData);
	if (!bUsedEyeGaze)
	{
		GazeData.GazeOrigin = HeadPose.GetLocation();
		GazeData.GazeDirection = HeadPose.GetRotation().GetForwardVector();
	}

	FPlane HandPlane;
	FVector ActivationPoint;
	if (!GetHandPlaneAndActivationPoint(NewHand, Zone, HandPlane, ActivationPoint))
	{
		return false;
	}

	if (FMath::IsNearlyZero(FVector::DotProduct(HandPlane.GetSafeNormal(), GazeData.GazeDirection)))
	{
		return false;
	}

	const FVector GazePositionOnPlane = FMath::RayPlaneIntersection(GazeData.GazeOrigin, GazeData.GazeDirection, HandPlane);
	const float DistanceToActivationPoint = FVector::DistSquared(ActivationPoint, GazePositionOnPlane);
	const float ActivationThreshold = FMath::Square(bUsedEyeGaze ? EyeGazeProximityThreshold : HeadGazeProximityThreshold);

	// Accept the hand if the eye / head gaze is within the threshold
	return DistanceToActivationPoint <= ActivationThreshold;
}
