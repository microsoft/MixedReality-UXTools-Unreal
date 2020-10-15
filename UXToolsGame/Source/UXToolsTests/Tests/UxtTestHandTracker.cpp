// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTestHandTracker.h"

FUxtTestHandData::FUxtTestHandData()
{
	for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
	{
		JointOrientation[i] = FQuat::Identity;
		JointPosition[i] = FVector::ZeroVector;
		JointRadius[i] = 1.0f;
	}
}

bool FUxtTestHandTracker::GetJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	const FUxtTestHandData& HandState = GetHandState(Hand);
	if (HandState.bIsTracked)
	{
		OutOrientation = HandState.JointOrientation[(uint8)Joint];
		OutPosition = HandState.JointPosition[(uint8)Joint];
		OutRadius = HandState.JointRadius[(uint8)Joint];
		return true;
	}

	return false;
}

bool FUxtTestHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	const FUxtTestHandData& HandState = GetHandState(Hand);
	if (HandState.bIsTracked)
	{
		OutOrientation = HandState.JointOrientation[(uint8)EUxtHandJoint::IndexProximal];
		OutPosition = HandState.JointPosition[(uint8)EUxtHandJoint::IndexProximal];
		return true;
	}

	return false;
}

bool FUxtTestHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	const FUxtTestHandData& HandState = GetHandState(Hand);
	if (HandState.bIsTracked)
	{
		OutIsGrabbing = HandState.bIsGrabbing;
		return true;
	}

	return false;
}

bool FUxtTestHandTracker::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	const FUxtTestHandData& HandState = GetHandState(Hand);
	if (HandState.bIsTracked)
	{
		OutIsSelectPressed = HandState.bIsSelectPressed;
		return true;
	}

	return false;
}

const FUxtTestHandData& FUxtTestHandTracker::GetHandState(EControllerHand Hand) const
{
	switch (Hand)
	{
	default:
	case EControllerHand::Left:
		return LeftHandData;
	case EControllerHand::Right:
		return RightHandData;
	}
}

void FUxtTestHandTracker::SetTracked(bool bIsTracked, EControllerHand Hand)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		LeftHandData.bIsTracked = bIsTracked;
		break;
	case EControllerHand::Right:
		RightHandData.bIsTracked = bIsTracked;
		break;
	case EControllerHand::AnyHand:
		LeftHandData.bIsTracked = bIsTracked;
		RightHandData.bIsTracked = bIsTracked;
		break;
	}
}

void FUxtTestHandTracker::SetGrabbing(bool bIsGrabbing, EControllerHand Hand)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		LeftHandData.bIsGrabbing = bIsGrabbing;
		break;
	case EControllerHand::Right:
		RightHandData.bIsGrabbing = bIsGrabbing;
		break;
	case EControllerHand::AnyHand:
		LeftHandData.bIsGrabbing = bIsGrabbing;
		RightHandData.bIsGrabbing = bIsGrabbing;
		break;
	}
}

void FUxtTestHandTracker::SetSelectPressed(bool bIsSelectPressed, EControllerHand Hand)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		LeftHandData.bIsSelectPressed = bIsSelectPressed;
		break;
	case EControllerHand::Right:
		RightHandData.bIsSelectPressed = bIsSelectPressed;
		break;
	case EControllerHand::AnyHand:
		LeftHandData.bIsSelectPressed = bIsSelectPressed;
		RightHandData.bIsSelectPressed = bIsSelectPressed;
		break;
	}
}

void FUxtTestHandTracker::SetJointPosition(const FVector& Position, EControllerHand Hand, EUxtHandJoint Joint)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		LeftHandData.JointPosition[(uint8)Joint] = Position;
		break;
	case EControllerHand::Right:
		RightHandData.JointPosition[(uint8)Joint] = Position;
		break;
	case EControllerHand::AnyHand:
		LeftHandData.JointPosition[(uint8)Joint] = Position;
		RightHandData.JointPosition[(uint8)Joint] = Position;
		break;
	}
}

void FUxtTestHandTracker::SetAllJointPositions(const FVector& Position, EControllerHand Hand)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			LeftHandData.JointPosition[i] = Position;
		}
		break;
	case EControllerHand::Right:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			RightHandData.JointPosition[i] = Position;
		}
		break;
	case EControllerHand::AnyHand:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			LeftHandData.JointPosition[i] = Position;
			RightHandData.JointPosition[i] = Position;
		}
		break;
	}
}

void FUxtTestHandTracker::SetJointOrientation(const FQuat& Orientation, EControllerHand Hand, EUxtHandJoint Joint)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		LeftHandData.JointOrientation[(uint8)Joint] = Orientation;
		break;
	case EControllerHand::Right:
		RightHandData.JointOrientation[(uint8)Joint] = Orientation;
		break;
	case EControllerHand::AnyHand:
		LeftHandData.JointOrientation[(uint8)Joint] = Orientation;
		RightHandData.JointOrientation[(uint8)Joint] = Orientation;
		break;
	}
}

void FUxtTestHandTracker::SetAllJointOrientations(const FQuat& Orientation, EControllerHand Hand)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			LeftHandData.JointOrientation[i] = Orientation;
		}
		break;
	case EControllerHand::Right:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			RightHandData.JointOrientation[i] = Orientation;
		}
		break;
	case EControllerHand::AnyHand:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			LeftHandData.JointOrientation[i] = Orientation;
			RightHandData.JointOrientation[i] = Orientation;
		}
		break;
	}
}

void FUxtTestHandTracker::SetJointRadius(float Radius, EControllerHand Hand, EUxtHandJoint Joint)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		LeftHandData.JointRadius[(uint8)Joint] = Radius;
		break;
	case EControllerHand::Right:
		RightHandData.JointRadius[(uint8)Joint] = Radius;
		break;
	case EControllerHand::AnyHand:
		LeftHandData.JointRadius[(uint8)Joint] = Radius;
		RightHandData.JointRadius[(uint8)Joint] = Radius;
		break;
	}
}

void FUxtTestHandTracker::SetAllJointRadii(float Radius, EControllerHand Hand)
{
	switch (Hand)
	{
	case EControllerHand::Left:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			LeftHandData.JointRadius[i] = Radius;
		}
		break;
	case EControllerHand::Right:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			RightHandData.JointRadius[i] = Radius;
		}
		break;
	case EControllerHand::AnyHand:
		for (uint8 i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
		{
			LeftHandData.JointRadius[i] = Radius;
			RightHandData.JointRadius[i] = Radius;
		}
		break;
	}
}
