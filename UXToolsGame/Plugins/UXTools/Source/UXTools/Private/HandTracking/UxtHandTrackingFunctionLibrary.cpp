// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

#include "Features/IModularFeatures.h"

bool UUxtHandTrackingFunctionLibrary::GetHandJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetJointState(Hand, Joint, OutOrientation, OutPosition, OutRadius);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetHandPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetPointerPose(Hand, OutOrientation, OutPosition);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(EControllerHand Hand, bool& OutIsGrabbing)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetIsGrabbing(Hand, OutIsGrabbing);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetIsSelectPressed(Hand, OutIsSelectPressed);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::IsHandTracked(EControllerHand Hand)
{
	bool NotUsed = false;
	return GetIsHandGrabbing(Hand, NotUsed);
}
