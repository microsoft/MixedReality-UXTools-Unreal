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
	const FKey Key_Left_Select("MicrosoftHandInteraction_Left_Select_Axis");
	const FKey Key_Left_Squeeze("MicrosoftHandInteraction_Left_Squeeze_Axis");
	const FKey Key_Right_Select("MicrosoftHandInteraction_Right_Select_Axis");
	const FKey Key_Right_Squeeze("MicrosoftHandInteraction_Right_Squeeze_Axis");

	const FName Axis_Left_Select = TEXT("UxtDefaultHandTrackerAxis_LeftSelect");
	const FName Axis_Left_Grab = TEXT("UxtDefaultHandTrackerAxis_LeftGrab");
	const FName Axis_Right_Select = TEXT("UxtDefaultHandTrackerAxis_RightSelect");
	const FName Axis_Right_Grab = TEXT("UxtDefaultHandTrackerAxis_RightGrab");

	const FInputAxisKeyMapping AxisMapping_Left_Select(Axis_Left_Select, Key_Left_Select);
	const FInputAxisKeyMapping AxisMapping_Left_Grab(Axis_Left_Grab, Key_Left_Squeeze);
	const FInputAxisKeyMapping AxisMapping_Right_Select(Axis_Right_Select, Key_Right_Select);
	const FInputAxisKeyMapping AxisMapping_Right_Grab(Axis_Right_Grab, Key_Right_Squeeze);

	void RegisterActions()
	{
		UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
		if (!InputSettings)
		{
			UE_LOG(LogUxtDefaultHandTracker, Warning, TEXT("Could not find mutable input settings"));
			return;
		}

		InputSettings->AddAxisMapping(AxisMapping_Left_Select);
		InputSettings->AddAxisMapping(AxisMapping_Left_Grab);
		InputSettings->AddAxisMapping(AxisMapping_Right_Select);
		InputSettings->AddAxisMapping(AxisMapping_Right_Grab);
	}

	void UnregisterActions()
	{
		UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
		if (!InputSettings)
		{
			return;
		}

		InputSettings->RemoveAxisMapping(AxisMapping_Left_Select);
		InputSettings->RemoveAxisMapping(AxisMapping_Left_Grab);
		InputSettings->RemoveAxisMapping(AxisMapping_Right_Select);
		InputSettings->RemoveAxisMapping(AxisMapping_Right_Grab);
	}

	/** Threshold for activating and releasing actions.
	* Not very relevant for Select/Squeeze axes currently
	* because they only take on 0.0 and 1.0 values.
	*/
	const float AxisActivateThreshold = 0.8f;
	const float AxisReleaseThreshold = 0.5f;

}

bool FUxtDefaultHandTracker::GetJointState(
	EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	const FXRMotionControllerData& HandData = (Hand == EControllerHand::Left ? ControllerData_Left : ControllerData_Right);
	if (HandData.bValid)
	{
		OutOrientation = HandData.HandKeyRotations[(int32)Joint];
		OutPosition = HandData.HandKeyPositions[(int32)Joint];
		OutRadius = HandData.HandKeyRadii[(int32)Joint];
		return true;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	const FXRMotionControllerData& HandData = (Hand == EControllerHand::Left ? ControllerData_Left : ControllerData_Right);
	if (HandData.bValid)
	{
		OutOrientation = HandData.AimRotation;
		OutPosition = HandData.AimPosition;
		return true;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	switch (Hand)
	{
	case EControllerHand::Left:
		return bIsGrabbing_Left;
	case EControllerHand::Right:
		return bIsGrabbing_Right;
	}
	return false;
}

bool FUxtDefaultHandTracker::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	switch (Hand)
	{
	case EControllerHand::Left:
		return bIsSelectPressed_Left;
	case EControllerHand::Right:
		return bIsSelectPressed_Right;
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
			NewPlayer->InputComponent->BindAxis(Axis_Left_Grab, this, &UUxtDefaultHandTrackerSubsystem::OnLeftGrab);
			NewPlayer->InputComponent->BindAxis(Axis_Right_Select, this, &UUxtDefaultHandTrackerSubsystem::OnRightSelect);
			NewPlayer->InputComponent->BindAxis(Axis_Right_Grab, this, &UUxtDefaultHandTrackerSubsystem::OnRightGrab);
		}

		TickDelegateHandle =
			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UUxtDefaultHandTrackerSubsystem::Tick));

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

			FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);

			if (PlayerController->InputComponent)
			{
				PlayerController->InputComponent->AxisBindings.RemoveAll(
					[this](const FInputAxisBinding& Binding) -> bool { return Binding.AxisDelegate.IsBoundToObject(this); });
			}
		}
	}
}

bool UUxtDefaultHandTrackerSubsystem::Tick(float DeltaSeconds)
{
	if (IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get())
	{
		if (UWorld* World = GetLocalPlayer()->GetWorld())
		{
			XRSystem->GetMotionControllerData(GetLocalPlayer(), EControllerHand::Left, DefaultHandTracker.ControllerData_Left);
			XRSystem->GetMotionControllerData(GetLocalPlayer(), EControllerHand::Right, DefaultHandTracker.ControllerData_Right);
		}
	}
	return true;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftSelect(float AxisValue)
{
	//UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("LeftSelect: %f"), AxisValue);
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

void UUxtDefaultHandTrackerSubsystem::OnLeftGrab(float AxisValue)
{
	//UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("LeftGrab: %f"), AxisValue);
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
	//UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("RightSelect: %f"), AxisValue);
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

void UUxtDefaultHandTrackerSubsystem::OnRightGrab(float AxisValue)
{
	//UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("RightGrab: %f"), AxisValue);
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
