#include "UxtTestHandTracker.h"


bool FUxtTestHandTracker::GetJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	if (bIsTracked)
	{
		OutOrientation = TestOrientation;
		OutPosition = TestPosition;
		OutRadius = TestRadius;
		return true;
	}

	return false;
}

bool FUxtTestHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	OutOrientation = TestOrientation;
	OutPosition = TestPosition;
	return true;
}

bool FUxtTestHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	if (bIsTracked)
	{
		OutIsGrabbing = bIsGrabbing;
		return true;
	}

	return false;
}