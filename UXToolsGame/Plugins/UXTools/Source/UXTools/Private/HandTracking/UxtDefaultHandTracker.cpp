// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtDefaultHandTracker.h"

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

}

void UUxtDefaultHandTracker::Initialize(FSubsystemCollectionBase& Collection)
{
	RegisterActions();

	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtDefaultHandTracker::OnGameModePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtDefaultHandTracker::OnGameModeLogout);
}

void UUxtDefaultHandTracker::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	PostLoginHandle.Reset();
	LogoutHandle.Reset();

	UnregisterActions();
}

void UUxtDefaultHandTracker::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->Player == GetLocalPlayer())
	{
		if (NewPlayer->InputComponent)
		{
			NewPlayer->InputComponent->BindAxis(Axis_Left_Select, this, &UUxtDefaultHandTracker::OnLeftSelect);
			NewPlayer->InputComponent->BindAxis(Axis_Left_Grab, this, &UUxtDefaultHandTracker::OnLeftGrab);
			NewPlayer->InputComponent->BindAxis(Axis_Right_Select, this, &UUxtDefaultHandTracker::OnRightSelect);
			NewPlayer->InputComponent->BindAxis(Axis_Right_Grab, this, &UUxtDefaultHandTracker::OnRightGrab);
		}

		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), this);
	}
}

void UUxtDefaultHandTracker::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (PlayerController->Player == GetLocalPlayer())
		{
			IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), this);

			if (PlayerController->InputComponent)
			{
				PlayerController->InputComponent->AxisBindings.RemoveAll([this](const FInputAxisBinding& Binding) -> bool
					{
						return Binding.AxisDelegate.IsBoundToObject(this);
					});
			}
		}
	}
}

void UUxtDefaultHandTracker::OnLeftSelect(float AxisValue)
{
	UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("LeftSelect: %f"), AxisValue);
}

void UUxtDefaultHandTracker::OnLeftGrab(float AxisValue)
{
	UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("LeftGrab: %f"), AxisValue);
}

void UUxtDefaultHandTracker::OnRightSelect(float AxisValue)
{
	UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("RightSelect: %f"), AxisValue);
}

void UUxtDefaultHandTracker::OnRightGrab(float AxisValue)
{
	UE_LOG(LogUxtDefaultHandTracker, Display, TEXT("RightGrab: %f"), AxisValue);
}

bool UUxtDefaultHandTracker::GetJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	// EWMRHandKeypoint Keypoint = (EWMRHandKeypoint)Joint;
	// FTransform Transform;

	// if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Keypoint, Transform, OutRadius))
	//{
	//	OutOrientation = Transform.GetRotation();
	//	OutPosition = Transform.GetTranslation();
	//	return true;
	//}

	return false;
}

bool UUxtDefaultHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	// FPointerPoseInfo Info = UWindowsMixedRealityFunctionLibrary::GetPointerPoseInfo(Hand);

	// if (Info.TrackingStatus != EHMDTrackingStatus::NotTracked)
	//{
	//	OutOrientation = Info.Orientation;
	//	OutPosition = Info.Origin;
	//	return true;
	//}

	return false;
}

bool UUxtDefaultHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
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

bool UUxtDefaultHandTracker::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
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
