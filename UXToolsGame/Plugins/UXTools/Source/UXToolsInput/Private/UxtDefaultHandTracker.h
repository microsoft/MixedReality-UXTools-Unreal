// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Engine/LocalPlayer.h"

#include "HeadMountedDisplayTypes.h"

#include "HandTracking/IUxtHandTracker.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputSubsystemInterface.h"
#include "InputMappingContext.h"

class AXRSimulationActor;
struct FXRSimulationState;

/** Default hand tracker implementation.
 *
 * This implementation works for all XR systems. It uses the XRTrackingSystem engine API.
 * Hand and controller data is based on the FXRMotionControllerData.
 *
 * Motion controller data is cached at the beginning of each frame.
 * Input events for known XR systems are used to keep track of Select and Grip actions.
 */
class FUxtDefaultHandTracker : public IUxtHandTracker
{
public:
	void RegisterInputMappings(UInputMappingContext* InputContext);
	void UnregisterInputMappings(UInputMappingContext* InputContext);

	FXRMotionControllerData& GetControllerData(EControllerHand Hand);
	const FXRMotionControllerData& GetControllerData(EControllerHand Hand) const;

	//
	// IUxtHandTracker interface

	virtual ETrackingStatus GetTrackingStatus(EControllerHand Hand) const override;
	virtual bool IsHandController(EControllerHand Hand) const override;
	virtual bool GetJointState(
		EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const override;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const override;
	virtual bool GetGripPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const override;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const override;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const override;

private:
	FXRMotionControllerData ControllerData_Left;
	FXRMotionControllerData ControllerData_Right;
	bool bIsGrabbing_Left = false;
	bool bIsSelectPressed_Left = false;
	bool bIsGrabbing_Right = false;
	bool bIsSelectPressed_Right = false;

	friend class UUxtDefaultHandTrackerSubsystem;

private:
	void RegisterEnhancedInputAction(UInputMappingContext* InputContext, UInputAction*& Action, FText Description, TArray<FKey> Keys);

public:
	UInputAction* LeftSelect;
	UInputAction* LeftGrab;
	UInputAction* RightSelect;
	UInputAction* RightGrab;
};
