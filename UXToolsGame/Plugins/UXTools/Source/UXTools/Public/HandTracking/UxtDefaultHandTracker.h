// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "HandTracking/IUxtHandTracker.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "UxtDefaultHandTracker.generated.h"

/** Default hand tracker implementation. */
class FUxtDefaultHandTracker : public IUxtHandTracker
{
public:
	//
	// IUxtHandTracker interface

	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const;

public:
	bool bIsGrabbing_Left = false;
	bool bIsSelectPressed_Left = false;
	bool bIsGrabbing_Right = false;
	bool bIsSelectPressed_Right = false;
};

/** Subsystem for registering the default hand tracker. */
UCLASS()
class UUxtDefaultHandTrackerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

	void OnLeftSelect(float AxisValue);
	void OnLeftGrab(float AxisValue);
	void OnRightSelect(float AxisValue);
	void OnRightGrab(float AxisValue);

private:
	FUxtDefaultHandTracker DefaultHandTracker;

	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;
};
