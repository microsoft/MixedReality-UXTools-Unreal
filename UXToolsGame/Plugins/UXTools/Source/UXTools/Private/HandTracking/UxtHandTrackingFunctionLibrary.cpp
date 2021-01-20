// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

#include "Features/IModularFeatures.h"

bool UUxtHandTrackingFunctionLibrary::GetHandJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius)
{
	return IUxtHandTracker::Get().GetJointState(Hand, Joint, OutOrientation, OutPosition, OutRadius);
}

bool UUxtHandTrackingFunctionLibrary::GetHandPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition)
{
	return IUxtHandTracker::Get().GetPointerPose(Hand, OutOrientation, OutPosition);
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(EControllerHand Hand, bool& OutIsGrabbing)
{
	return IUxtHandTracker::Get().GetIsGrabbing(Hand, OutIsGrabbing);
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed)
{
	return IUxtHandTracker::Get().GetIsSelectPressed(Hand, OutIsSelectPressed);
}

bool UUxtHandTrackingFunctionLibrary::IsHandTracked(EControllerHand Hand)
{
	bool NotUsed = false;
	return GetIsHandGrabbing(Hand, NotUsed);
}
