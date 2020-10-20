// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IMotionController.h"

/**
 * Enum for hand joints.
 */
UENUM(BlueprintType)
enum class EUxtHandJoint : uint8
{
	Palm,
	Wrist,
	ThumbMetacarpal,
	ThumbProximal,
	ThumbDistal,
	ThumbTip,
	IndexMetacarpal,
	IndexProximal,
	IndexIntermediate,
	IndexDistal,
	IndexTip,
	MiddleMetacarpal,
	MiddleProximal,
	MiddleIntermediate,
	MiddleDistal,
	MiddleTip,
	RingMetacarpal,
	RingProximal,
	RingIntermediate,
	RingDistal,
	RingTip,
	LittleMetacarpal,
	LittleProximal,
	LittleIntermediate,
	LittleDistal,
	LittleTip,

	Count UMETA(Hidden, DisplayName = "<INVALID>")
};

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
	static IUxtHandTracker* GetHandTracker();

	virtual ~IUxtHandTracker() {}

	/** Obtain the state of the given joint. Returns false if the hand is not tracked this frame, in which case the values of the output
	 * parameters are unchanged. */
	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const = 0;

	/** Obtain the pointer pose. Returns false if the hand is not tracked this frame, in which case the value of the output parameter is
	 * unchanged. */
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const = 0;

	/** Obtain current grabbing state. Returns false if the hand is not tracked this frame, in which case the value of the output parameter
	 * is unchanged. */
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const = 0;

	/** Obtain current selection state. Returns false if the hand is not tracked this frame, in which case the value of the output parameter
	 * is unchanged. */
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const = 0;
};
