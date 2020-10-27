// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "HandTracking/IUxtHandTracker.h"
#include "Subsystems/LocalPlayerSubsystem.h"

/** Default hand tracker implementation. */
class UUxtDefaultHandTracker
	: public IUxtHandTracker
	, public ULocalPlayerSubsystem
{
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//
	// IUxtHandTracker interface

	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const;

private:
	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

	void OnLeftSelect(float AxisValue);
	void OnLeftGrab(float AxisValue);
	void OnRightSelect(float AxisValue);
	void OnRightGrab(float AxisValue);

private:
	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;

	bool bIsGrabbing_Left = false;
	bool bIsSelectPressed_Left = false;
	bool bIsGrabbing_Right = false;
	bool bIsSelectPressed_Right = false;
};
