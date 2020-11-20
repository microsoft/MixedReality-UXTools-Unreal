// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "HeadMountedDisplayTypes.h"

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
		EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const;

public:
	FXRMotionControllerData ControllerData_Left;
	FXRMotionControllerData ControllerData_Right;
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

	void OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime);

	void OnLeftSelect(float AxisValue);
	void OnLeftGrip(float AxisValue);
	void OnRightSelect(float AxisValue);
	void OnRightGrip(float AxisValue);

private:
	FUxtDefaultHandTracker DefaultHandTracker;

	FDelegateHandle TickDelegateHandle;

	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;
};
