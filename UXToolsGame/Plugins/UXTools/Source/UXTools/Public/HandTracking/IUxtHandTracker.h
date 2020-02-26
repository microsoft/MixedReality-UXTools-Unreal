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
	Pointer		// special joint used to obtain the pointer pose
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

	static FName GetModularFeatureName()
	{
		static FName FeatureName = FName(TEXT("UxtHandTracker"));
		return FeatureName;
	}

	virtual ~IUxtHandTracker() {}

	/** Obtain state of the given joint. Returns false if the hand is not tracked this frame, in which case the values of the output parameters are undefined. */
	virtual bool GetJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) = 0;

	/** Obtain current grabbing state. Returns false if the hand is not tracked this frame, in which case the value of the output parameter is undefined. */
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) = 0;
};