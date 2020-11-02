// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Behaviors/UxtPalmUpConstraintComponent.h"

#include "DrawDebugHelpers.h"

#include "Engine/World.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Utils/UxtFunctionLibrary.h"

bool UUxtPalmUpConstraintComponent::IsHandUsableForConstraint(EControllerHand NewHand) const
{
	FQuat PalmRotation;
	FVector PalmLocation;
	float PalmRadius;
	if (!UUxtHandTrackingFunctionLibrary::GetHandJointState(NewHand, EHandKeypoint::Palm, PalmRotation, PalmLocation, PalmRadius))
	{
		return false;
	}
	// Note: Palm Z is normal to the back of the hand, not the inside
	FVector PalmUpVector = PalmRotation.GetUpVector();

	// Test palm normal against camera view angle
	{
		FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());
		FVector LookAtVector = PalmLocation - HeadPose.GetLocation();
		LookAtVector.Normalize();

		float CosAngle = FVector::DotProduct(LookAtVector, PalmUpVector);
		const float MinCosAngle = FMath::Cos(FMath::DegreesToRadians(MaxPalmAngle));

		// Accept the hand if palm angle is within the cone limit
		if (CosAngle < MinCosAngle)
		{
			return false;
		}
	}

	// Test Palm-Index-Ring triangle against palm for measuring flatness
	if (bRequireFlatHand)
	{
		FQuat IndexRotation, RingRotation;
		FVector IndexLocation, RingLocation;
		float IndexRadius, RingRadius;
		if (!UUxtHandTrackingFunctionLibrary::GetHandJointState(
				NewHand, EHandKeypoint::IndexTip, IndexRotation, IndexLocation, IndexRadius) ||
			!UUxtHandTrackingFunctionLibrary::GetHandJointState(NewHand, EHandKeypoint::RingTip, RingRotation, RingLocation, RingRadius))
		{
			return false;
		}

		FVector FingerNormal = FVector::CrossProduct(RingLocation - PalmLocation, IndexLocation - PalmLocation);
		if (NewHand != EControllerHand::Left)
		{
			FingerNormal = -FingerNormal;
		}
		FingerNormal.Normalize();
		float CosFlatAngle = FVector::DotProduct(FingerNormal.GetSafeNormal(), PalmUpVector);
		float MinCosFlagAngle = FMath::Cos(FMath::DegreesToRadians(MaxFlatHandAngle));

		if (CosFlatAngle < MinCosFlagAngle)
		{
			return false;
		}
	}

	return true;
}
