// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "HandTracking/IUxtHandTracker.h"

/** Default hand tracker implementation. */
class FUxtDefaultHandTracker : public IUxtHandTracker
{
public:
	static void RegisterActions();
	static void UnregisterActions();

	//
	// IUxtHandTracker interface

	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const;

private:
	bool bIsGrabbing_Left = false;
	bool bIsSelectPressed_Left = false;
	bool bIsGrabbing_Right = false;
	bool bIsSelectPressed_Right = false;
};
