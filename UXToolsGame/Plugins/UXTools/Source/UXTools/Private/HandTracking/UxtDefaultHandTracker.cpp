// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtDefaultHandTracker.h"

#include "ARSupportInterface.h"
#include "IXRTrackingSystem.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Features/IModularFeatures.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "Utils/UxtFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogUxtDefaultHandTracker, Log, All);

namespace
{
	const FKey Key_Left_Select("OpenXRMsftHandInteraction_Left_Select_Axis");
	const FKey Key_Left_Grip("OpenXRMsftHandInteraction_Left_Grip_Axis");
	const FKey Key_Right_Select("OpenXRMsftHandInteraction_Right_Select_Axis");
	const FKey Key_Right_Grip("OpenXRMsftHandInteraction_Right_Grip_Axis");

	const FName Axis_Left_Select = TEXT("UxtDefaultHandTrackerAxis_LeftSelect");
	const FName Axis_Left_Grip = TEXT("UxtDefaultHandTrackerAxis_LeftGrip");
	const FName Axis_Right_Select = TEXT("UxtDefaultHandTrackerAxis_RightSelect");
	const FName Axis_Right_Grip = TEXT("UxtDefaultHandTrackerAxis_RightGrip");

	const FInputAxisKeyMapping AxisMapping_Left_Select(Axis_Left_Select, Key_Left_Select);
	const FInputAxisKeyMapping AxisMapping_Left_Grip(Axis_Left_Grip, Key_Left_Grip);
	const FInputAxisKeyMapping AxisMapping_Right_Select(Axis_Right_Select, Key_Right_Select);
	const FInputAxisKeyMapping AxisMapping_Right_Grip(Axis_Right_Grip, Key_Right_Grip);

	void RegisterActions()
	{
		UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
		if (!InputSettings)
		{
			UE_LOG(LogUxtDefaultHandTracker, Warning, TEXT("Could not find mutable input settings"));
			return;
		}

		InputSettings->AddAxisMapping(AxisMapping_Left_Select);
		InputSettings->AddAxisMapping(AxisMapping_Left_Grip);
		InputSettings->AddAxisMapping(AxisMapping_Right_Select);
		InputSettings->AddAxisMapping(AxisMapping_Right_Grip);
	}

	void UnregisterActions()
	{
		UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
		if (!InputSettings)
		{
			return;
		}

		InputSettings->RemoveAxisMapping(AxisMapping_Left_Select);
		InputSettings->RemoveAxisMapping(AxisMapping_Left_Grip);
		InputSettings->RemoveAxisMapping(AxisMapping_Right_Select);
		InputSettings->RemoveAxisMapping(AxisMapping_Right_Grip);
	}

	/** Threshold for activating and releasing actions.
	* Not very relevant for Select/Squeeze axes currently
	* because they only take on 0.0 and 1.0 values.
	*/
	const float AxisActivateThreshold = 0.8f;
	const float AxisReleaseThreshold = 0.5f;

	bool IsValidHandData(const FXRMotionControllerData& MotionControllerData)
	{
		if (MotionControllerData.DeviceVisualType == EXRVisualType::Hand && MotionControllerData.bValid)
		{
			check(
				MotionControllerData.HandKeyPositions.Num() == EHandKeypointCount &&
				MotionControllerData.HandKeyRotations.Num() == EHandKeypointCount &&
				MotionControllerData.HandKeyRadii.Num() == EHandKeypointCount);
			return true;
		}
		return false;
	}

}

FXRMotionControllerData& FUxtDefaultHandTracker::GetControllerData(EControllerHand Hand)
{
	return Hand == EControllerHand::Left ? ControllerData_Left : ControllerData_Right;
}

const FXRMotionControllerData& FUxtDefaultHandTracker::GetControllerData(EControllerHand Hand) const
{
	return Hand == EControllerHand::Left ? ControllerData_Left : ControllerData_Right;
}

ETrackingStatus FUxtDefaultHandTracker::GetTrackingStatus(EControllerHand Hand) const
{
	const FXRMotionControllerData& MotionControllerData = GetControllerData(Hand);
	return MotionControllerData.bValid ? MotionControllerData.TrackingStatus : ETrackingStatus::NotTracked;
}

bool FUxtDefaultHandTracker::HasHandData(EControllerHand Hand) const
{
	const FXRMotionControllerData& MotionControllerData = GetControllerData(Hand);
	return IsValidHandData(MotionControllerData);
}

bool FUxtDefaultHandTracker::GetJointState(
	EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	const FXRMotionControllerData& MotionControllerData = GetControllerData(Hand);
	if (IsValidHandData(MotionControllerData))
	{
		const int32 iJoint = (int32)Joint;
		OutOrientation = MotionControllerData.HandKeyRotations[iJoint];
		OutPosition = MotionControllerData.HandKeyPositions[iJoint];
		OutRadius = MotionControllerData.HandKeyRadii[iJoint];
		return true;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	const FXRMotionControllerData& MotionControllerData = GetControllerData(Hand);
	if (MotionControllerData.bValid)
	{
		OutOrientation = MotionControllerData.AimRotation;
		OutPosition = MotionControllerData.AimPosition;
		return true;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetGripPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	const FXRMotionControllerData& MotionControllerData = GetControllerData(Hand);
	if (MotionControllerData.bValid)
	{
		OutOrientation = MotionControllerData.GripRotation;
		OutPosition = MotionControllerData.GripPosition;
		return true;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	switch (Hand)
	{
	case EControllerHand::Left:
		OutIsGrabbing = bIsGrabbing_Left;
		return true;
	case EControllerHand::Right:
		OutIsGrabbing = bIsGrabbing_Right;
		return true;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	switch (Hand)
	{
	case EControllerHand::Left:
		OutIsSelectPressed = bIsSelectPressed_Left;
		return true;
	case EControllerHand::Right:
		OutIsSelectPressed = bIsSelectPressed_Right;
		return true;
	}
	return false;
}

void UUxtDefaultHandTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	RegisterActions();

	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModeLogout);
}

void UUxtDefaultHandTrackerSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	PostLoginHandle.Reset();
	LogoutHandle.Reset();

	UnregisterActions();
}

void UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->Player == GetLocalPlayer())
	{
		if (NewPlayer->InputComponent)
		{
			NewPlayer->InputComponent->BindAxis(Axis_Left_Select, this, &UUxtDefaultHandTrackerSubsystem::OnLeftSelect);
			NewPlayer->InputComponent->BindAxis(Axis_Left_Grip, this, &UUxtDefaultHandTrackerSubsystem::OnLeftGrip);
			NewPlayer->InputComponent->BindAxis(Axis_Right_Select, this, &UUxtDefaultHandTrackerSubsystem::OnRightSelect);
			NewPlayer->InputComponent->BindAxis(Axis_Right_Grip, this, &UUxtDefaultHandTrackerSubsystem::OnRightGrip);
		}

		TickDelegateHandle = FWorldDelegates::OnWorldTickStart.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnWorldTickStart);

		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &DefaultHandTracker);
	}
}

void UUxtDefaultHandTrackerSubsystem::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (PlayerController->Player == GetLocalPlayer())
		{
			IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &DefaultHandTracker);

			FWorldDelegates::OnWorldTickStart.Remove(TickDelegateHandle);

			if (PlayerController->InputComponent)
			{
				PlayerController->InputComponent->AxisBindings.RemoveAll(
					[this](const FInputAxisBinding& Binding) -> bool { return Binding.AxisDelegate.IsBoundToObject(this); });
			}
		}
	}
}

void UUxtDefaultHandTrackerSubsystem::OnWorldTickStart(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	if (IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get())
	{
		XRSystem->GetMotionControllerData(GetLocalPlayer(), EControllerHand::Left, DefaultHandTracker.ControllerData_Left);
		XRSystem->GetMotionControllerData(GetLocalPlayer(), EControllerHand::Right, DefaultHandTracker.ControllerData_Right);

		// XXX HACK: WMR always reporting invalid hand data, remove once fixed!
		DefaultHandTracker.ControllerData_Left.bValid = true;
		DefaultHandTracker.ControllerData_Right.bValid = true;
		// XXX
	}
}

void UUxtDefaultHandTrackerSubsystem::OnLeftSelect(float AxisValue)
{
	if (!DefaultHandTracker.bIsSelectPressed_Left)
	{
		if (AxisValue > AxisActivateThreshold)
		{
			DefaultHandTracker.bIsSelectPressed_Left = true;
		}
	}
	else
	{
		if (AxisValue < AxisReleaseThreshold)
		{
			DefaultHandTracker.bIsSelectPressed_Left = false;
		}
	}
}

void UUxtDefaultHandTrackerSubsystem::OnLeftGrip(float AxisValue)
{
	if (!DefaultHandTracker.bIsGrabbing_Left)
	{
		if (AxisValue > AxisActivateThreshold)
		{
			DefaultHandTracker.bIsGrabbing_Left = true;
		}
	}
	else
	{
		if (AxisValue < AxisReleaseThreshold)
		{
			DefaultHandTracker.bIsGrabbing_Left = false;
		}
	}
}

void UUxtDefaultHandTrackerSubsystem::OnRightSelect(float AxisValue)
{
	if (!DefaultHandTracker.bIsSelectPressed_Right)
	{
		if (AxisValue > AxisActivateThreshold)
		{
			DefaultHandTracker.bIsSelectPressed_Right = true;
		}
	}
	else
	{
		if (AxisValue < AxisReleaseThreshold)
		{
			DefaultHandTracker.bIsSelectPressed_Right = false;
		}
	}
}

void UUxtDefaultHandTrackerSubsystem::OnRightGrip(float AxisValue)
{
	if (!DefaultHandTracker.bIsGrabbing_Right)
	{
		if (AxisValue > AxisActivateThreshold)
		{
			DefaultHandTracker.bIsGrabbing_Right = true;
		}
	}
	else
	{
		if (AxisValue < AxisReleaseThreshold)
		{
			DefaultHandTracker.bIsGrabbing_Right = false;
		}
	}
}
