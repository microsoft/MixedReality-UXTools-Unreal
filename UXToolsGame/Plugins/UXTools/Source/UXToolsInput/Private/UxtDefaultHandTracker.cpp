// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtDefaultHandTracker.h"

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

	const FName Action_Left_Select = TEXT("UxtDefaultHandTracker_LeftSelect");
	const FName Action_Left_Grip = TEXT("UxtDefaultHandTracker_LeftGrip");
	const FName Action_Right_Select = TEXT("UxtDefaultHandTracker_RightSelect");
	const FName Action_Right_Grip = TEXT("UxtDefaultHandTracker_RightGrip");

	const TArray<FInputActionKeyMapping> ActionMappings({
		FInputActionKeyMapping(Action_Left_Select, Key_Left_Select),
		FInputActionKeyMapping(Action_Left_Grip, Key_Left_Grip),
		FInputActionKeyMapping(Action_Right_Select, Key_Right_Select),
		FInputActionKeyMapping(Action_Right_Grip, Key_Right_Grip),
	});

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

} // namespace

void FUxtDefaultHandTracker::RegisterInputMappings()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		UE_LOG(LogUxtDefaultHandTracker, Warning, TEXT("Could not find mutable input settings"));
		return;
	}

	for (const FInputActionKeyMapping& Mapping : ActionMappings)
	{
		if (Mapping.Key.IsValid())
		{
			InputSettings->AddActionMapping(Mapping, false);
		}
	}
	InputSettings->ForceRebuildKeymaps();
}

void FUxtDefaultHandTracker::UnregisterInputMappings()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		return;
	}

	for (const FInputActionKeyMapping& Mapping : ActionMappings)
	{
		InputSettings->RemoveActionMapping(Mapping, false);
	}
	InputSettings->ForceRebuildKeymaps();
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
	FUxtDefaultHandTracker::RegisterInputMappings();

	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModeLogout);
}

void UUxtDefaultHandTrackerSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	PostLoginHandle.Reset();
	LogoutHandle.Reset();

	FUxtDefaultHandTracker::UnregisterInputMappings();
}

void UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->IsLocalController())
	{
		if (NewPlayer->InputComponent)
		{
			NewPlayer->InputComponent->BindAction(
				Action_Left_Select, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnLeftSelectPressed);
			NewPlayer->InputComponent->BindAction(
				Action_Left_Select, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnLeftSelectReleased);
			NewPlayer->InputComponent->BindAction(
				Action_Left_Grip, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnLeftGripPressed);
			NewPlayer->InputComponent->BindAction(
				Action_Left_Grip, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnLeftGripReleased);
			NewPlayer->InputComponent->BindAction(
				Action_Right_Select, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnRightSelectPressed);
			NewPlayer->InputComponent->BindAction(
				Action_Right_Select, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnRightSelectReleased);
			NewPlayer->InputComponent->BindAction(
				Action_Right_Grip, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnRightGripPressed);
			NewPlayer->InputComponent->BindAction(
				Action_Right_Grip, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnRightGripReleased);
		}

		TickDelegateHandle = FWorldDelegates::OnWorldTickStart.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnWorldTickStart);

		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &DefaultHandTracker);
	}
}

void UUxtDefaultHandTrackerSubsystem::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (PlayerController->IsLocalController())
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
		XRSystem->GetMotionControllerData(World, EControllerHand::Left, DefaultHandTracker.ControllerData_Left);
		XRSystem->GetMotionControllerData(World, EControllerHand::Right, DefaultHandTracker.ControllerData_Right);

		// XXX HACK: WMR always reporting invalid hand data, remove once fixed!
		DefaultHandTracker.ControllerData_Left.bValid = true;
		DefaultHandTracker.ControllerData_Right.bValid = true;
		DefaultHandTracker.ControllerData_Left.DeviceVisualType =
			DefaultHandTracker.ControllerData_Left.HandKeyPositions.Num() == EHandKeypointCount ? EXRVisualType::Hand
																								: EXRVisualType::Controller;
		DefaultHandTracker.ControllerData_Right.DeviceVisualType =
			DefaultHandTracker.ControllerData_Right.HandKeyPositions.Num() == EHandKeypointCount ? EXRVisualType::Hand
																								 : EXRVisualType::Controller;
		// XXX
	}
}

void UUxtDefaultHandTrackerSubsystem::OnLeftSelectPressed()
{
	DefaultHandTracker.bIsSelectPressed_Left = true;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftSelectReleased()
{
	DefaultHandTracker.bIsSelectPressed_Left = false;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftGripPressed()
{
	DefaultHandTracker.bIsGrabbing_Left = true;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftGripReleased()
{
	DefaultHandTracker.bIsGrabbing_Left = false;
}

void UUxtDefaultHandTrackerSubsystem::OnRightSelectPressed()
{
	DefaultHandTracker.bIsSelectPressed_Right = true;
}

void UUxtDefaultHandTrackerSubsystem::OnRightSelectReleased()
{
	DefaultHandTracker.bIsSelectPressed_Right = false;
}

void UUxtDefaultHandTrackerSubsystem::OnRightGripPressed()
{
	DefaultHandTracker.bIsGrabbing_Right = true;
}

void UUxtDefaultHandTrackerSubsystem::OnRightGripReleased()
{
	DefaultHandTracker.bIsGrabbing_Right = false;
}
