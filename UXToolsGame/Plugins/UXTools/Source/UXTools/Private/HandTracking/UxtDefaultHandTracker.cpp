// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtDefaultHandTracker.h"
#include "GameFramework/InputSettings.h"

#include "Utils/UxtFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogUxtDefaultHandTracker, Log, All);

namespace
{
	const FKey MicrosoftHandInteraction_Left_Select("MicrosoftHandInteraction_Left_Select_Axis");
	const FKey MicrosoftHandInteraction_Left_Squeeze("MicrosoftHandInteraction_Left_Squeeze_Axis");
	const FKey MicrosoftHandInteraction_Right_Select("MicrosoftHandInteraction_Right_Select_Axis");
	const FKey MicrosoftHandInteraction_Right_Squeeze("MicrosoftHandInteraction_Right_Squeeze_Axis");

	const FName ActionLeftSelect = TEXT("UxtDefaultHandTrackerAction_LeftSelect");
	const FName ActionLeftGrab = TEXT("UxtDefaultHandTrackerAction_LeftGrab");
	const FName ActionRightSelect = TEXT("UxtDefaultHandTrackerAction_RightSelect");
	const FName ActionRightGrab = TEXT("UxtDefaultHandTrackerAction_RightGrab");

	const FInputActionKeyMapping ActionMapping_Left_Select(ActionLeftSelect, MicrosoftHandInteraction_Left_Select);
	const FInputActionKeyMapping ActionMapping_Left_Grab(ActionLeftGrab, MicrosoftHandInteraction_Left_Squeeze);
	const FInputActionKeyMapping ActionMapping_Right_Select(ActionRightSelect, MicrosoftHandInteraction_Right_Select);
	const FInputActionKeyMapping ActionMapping_Right_Grab(ActionRightGrab, MicrosoftHandInteraction_Right_Squeeze);
} // namespace

void FUxtDefaultHandTracker::RegisterActions()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		UE_LOG(LogUxtDefaultHandTracker, Warning, TEXT("Could not find mutable input settings"));
		return;
	}

	InputSettings->AddActionMapping(ActionMapping_Left_Select);
	InputSettings->AddActionMapping(ActionMapping_Left_Grab);
	InputSettings->AddActionMapping(ActionMapping_Right_Select);
	InputSettings->AddActionMapping(ActionMapping_Right_Grab);

	//InputComponent->BindAction(Action_ToggleLeftHand, IE_Pressed, this, &AXRInputSimulationActor::OnToggleLeftHandPressed);
}

void FUxtDefaultHandTracker::UnregisterActions()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		return;
	}

	InputSettings->RemoveActionMapping(ActionMapping_Left_Select);
	InputSettings->RemoveActionMapping(ActionMapping_Left_Grab);
	InputSettings->RemoveActionMapping(ActionMapping_Right_Select);
	InputSettings->RemoveActionMapping(ActionMapping_Right_Grab);
}

bool FUxtDefaultHandTracker::GetJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	//EWMRHandKeypoint Keypoint = (EWMRHandKeypoint)Joint;
	//FTransform Transform;

	//if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Keypoint, Transform, OutRadius))
	//{
	//	OutOrientation = Transform.GetRotation();
	//	OutPosition = Transform.GetTranslation();
	//	return true;
	//}

	return false;
}

bool FUxtDefaultHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	//FPointerPoseInfo Info = UWindowsMixedRealityFunctionLibrary::GetPointerPoseInfo(Hand);

	//if (Info.TrackingStatus != EHMDTrackingStatus::NotTracked)
	//{
	//	OutOrientation = Info.Orientation;
	//	OutPosition = Info.Origin;
	//	return true;
	//}

	return false;
}

bool FUxtDefaultHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	//bool bTracked = UWindowsMixedRealityFunctionLibrary::GetControllerTrackingStatus(Hand) != EHMDTrackingStatus::NotTracked;

	//if (bTracked)
	//{
	//	OutIsGrabbing = UWindowsMixedRealityFunctionLibrary::IsGrasped(Hand);
	//}

	//return bTracked;
	return false;
}

bool FUxtDefaultHandTracker::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	//bool bTracked = UWindowsMixedRealityFunctionLibrary::GetControllerTrackingStatus(Hand) != EHMDTrackingStatus::NotTracked;

	//if (bTracked)
	//{
	//	OutIsSelectPressed = UWindowsMixedRealityFunctionLibrary::IsSelectPressed(Hand);
	//}

	//return bTracked;
	return false;
}
