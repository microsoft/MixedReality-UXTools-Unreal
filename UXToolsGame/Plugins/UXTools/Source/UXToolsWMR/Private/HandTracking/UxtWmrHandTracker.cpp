// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtWmrHandTracker.h"

#include "WindowsMixedRealityFunctionLibrary.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"

#include "Utils/UxtFunctionLibrary.h"

bool FUxtWmrHandTracker::GetJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	EWMRHandKeypoint Keypoint = (EWMRHandKeypoint)Joint;
	FTransform Transform;

	if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Keypoint, Transform, OutRadius))
	{
		OutOrientation = Transform.GetRotation();
		OutPosition = Transform.GetTranslation();
		return true;
	}

	return false;
}

bool FUxtWmrHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	FPointerPoseInfo Info = UWindowsMixedRealityFunctionLibrary::GetPointerPoseInfo(Hand);

	if (Info.TrackingStatus != EHMDTrackingStatus::NotTracked)
	{
		OutOrientation = Info.Orientation;
		OutPosition = Info.Origin;
		return true;
	}

	return false;
}

bool FUxtWmrHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	bool bTracked = UWindowsMixedRealityFunctionLibrary::GetControllerTrackingStatus(Hand) != EHMDTrackingStatus::NotTracked;

	if (bTracked)
	{
		OutIsGrabbing = UWindowsMixedRealityFunctionLibrary::IsGrasped(Hand);
	}

	return bTracked;
}

bool FUxtWmrHandTracker::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	bool bTracked = UWindowsMixedRealityFunctionLibrary::GetControllerTrackingStatus(Hand) != EHMDTrackingStatus::NotTracked;

	if (bTracked)
	{
		OutIsSelectPressed = UWindowsMixedRealityFunctionLibrary::IsSelectPressed(Hand);
	}

	return bTracked;
}
