#pragma once

#include "HandTracking/IUxtHandTracker.h"


/** WMR implementation of the hand tracker interface */
class FUxtWmrHandTracker : public IUxtHandTracker
{
public:

	// 
	// IUxtHandTracker interface

	virtual bool GetJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius);
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing);
};
