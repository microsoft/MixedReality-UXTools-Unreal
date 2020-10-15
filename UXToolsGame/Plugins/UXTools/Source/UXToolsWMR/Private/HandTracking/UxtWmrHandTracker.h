// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "HandTracking/IUxtHandTracker.h"

/** WMR implementation of the hand tracker interface */
class FUxtWmrHandTracker : public IUxtHandTracker
{
public:
	//
	// IUxtHandTracker interface

	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const;
};
