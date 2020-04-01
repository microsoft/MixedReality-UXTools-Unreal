#pragma once

#include "HandTracking/IUxtHandTracker.h"


/** WMR implementation of the hand tracker interface */
class FUxtTestHandTracker : public IUxtHandTracker
{
public:

	// 
	// IUxtHandTracker interface

	virtual bool GetJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const override;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const override;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const override;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const override;

	/** Enable hand tracking. */
	bool bIsTracked = true;

	/** Position vector used for all joints. */
	FVector TestPosition = FVector::ZeroVector;

	FQuat TestOrientation = FQuat::Identity;

	/** Radius used for joints. */
	float TestRadius = 1.0f;

	/** Enable grab state. */
	bool bIsGrabbing = false;

	/** Enable select state. */
	bool bIsSelectPressed = false;
};
