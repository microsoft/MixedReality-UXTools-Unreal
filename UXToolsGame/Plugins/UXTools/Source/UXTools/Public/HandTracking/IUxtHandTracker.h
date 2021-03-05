// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayTypes.h"
#include "IMotionController.h"

/**
 * Hand tracker device interface.
 * We assume that implementations poll and cache the hand tracking state at the beginning of the frame.
 * This allows us to assume that if a hand is reported as tracked it will remain so for the remainder of the frame,
 * simplifying client logic.
 */
class UXTOOLS_API IUxtHandTracker : public IModularFeature
{
public:
	static FName GetModularFeatureName();

	/** Returns the currently registered hand tracker or nullptr if none */
	static IUxtHandTracker& Get();

	virtual ~IUxtHandTracker() {}

	/** Get tracking status of the hand or motion controller. */
	virtual ETrackingStatus GetTrackingStatus(EControllerHand Hand) const = 0;

	/** True if the controller is a hand. */
	virtual bool IsHandController(EControllerHand Hand) const = 0;

	/** Obtain the state of the given joint.
	 * Returns false if the hand is not tracked this frame, in which case the values of the output parameters are unchanged.
	 */
	virtual bool GetJointState(
		EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const = 0;

	/** Obtain the pointer pose.
	 * Returns false if the hand is not tracked this frame, in which case the value of the output parameter is unchanged.
	 */
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const = 0;

	/** Grip pose following the controller.
	 * Returns false if the hand is not tracked this frame, in which case the value of the output parameter is unchanged.
	 */
	virtual bool GetGripPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const = 0;

	/** Obtain current grabbing state.
	 * Returns false if the hand is not tracked this frame, in which case the value of the output parameter is unchanged. */
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const = 0;

	/** Obtain current selection state.
	 * Returns false if the hand is not tracked this frame, in which case the value of the output parameter is unchanged. */
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const = 0;
};
