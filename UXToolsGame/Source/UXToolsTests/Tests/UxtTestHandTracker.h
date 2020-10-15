// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "InputCoreTypes.h"

#include "HandTracking/IUxtHandTracker.h"

struct FUxtTestHandData
{
	FUxtTestHandData();

	/** Enable tracking of this hand. */
	bool bIsTracked = true;

	/** Position for each joint. */
	FVector JointPosition[(uint8)EUxtHandJoint::Count];

	/** Rotation for each joint. */
	FQuat JointOrientation[(uint8)EUxtHandJoint::Count];

	/** Radius for each joint. */
	float JointRadius[(uint8)EUxtHandJoint::Count];

	/** Enable grab state. */
	bool bIsGrabbing = false;

	/** Enable select state. */
	bool bIsSelectPressed = false;
};

/** WMR implementation of the hand tracker interface */
class FUxtTestHandTracker : public IUxtHandTracker
{
public:
	//
	// IUxtHandTracker interface

	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const override;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const override;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const override;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const override;

	/** Get current hand state data. */
	const FUxtTestHandData& GetHandState(EControllerHand Hand) const;

	/** Set tracking status. */
	void SetTracked(bool bIsTracked, EControllerHand Hand = EControllerHand::AnyHand);

	/** Set grab state. */
	void SetGrabbing(bool bIsGrabbing, EControllerHand Hand = EControllerHand::AnyHand);

	/** Set select state. */
	void SetSelectPressed(bool bIsSelectPressed, EControllerHand Hand = EControllerHand::AnyHand);

	/** Set joint position. */
	void SetJointPosition(const FVector& Position, EControllerHand Hand, EUxtHandJoint Joint);

	/** Set position for all joints of the hand. */
	void SetAllJointPositions(const FVector& Position, EControllerHand Hand = EControllerHand::AnyHand);

	/** Set joint orientation. */
	void SetJointOrientation(const FQuat& Orientation, EControllerHand Hand, EUxtHandJoint Joint);

	/** Set orientation for all joints of the hand. */
	void SetAllJointOrientations(const FQuat& Orientation, EControllerHand Hand = EControllerHand::AnyHand);

	/** Set joint radius. */
	void SetJointRadius(float Radius, EControllerHand Hand, EUxtHandJoint Joint);

	/** Set radius for all joints of the hand. */
	void SetAllJointRadii(float Radius, EControllerHand Hand = EControllerHand::AnyHand);

private:
	/** Data for the left hand. */
	FUxtTestHandData LeftHandData;

	/** Data for the right hand. */
	FUxtTestHandData RightHandData;
};
