// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtDefaultHandTracker.h"

#include "Features/IModularFeatures.h"
#include "GameFramework/InputSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogUxtDefaultHandTracker, Log, All);

namespace
{
	const TArray<FInputActionKeyMapping> ActionMappings(
		{// OpenXR MsftHandInteraction mapping
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftSelect, FKey("OpenXRMsftHandInteraction_Left_Select_Axis")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftGrab, FKey("OpenXRMsftHandInteraction_Left_Grip_Axis")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightSelect, FKey("OpenXRMsftHandInteraction_Right_Select_Axis")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightGrab, FKey("OpenXRMsftHandInteraction_Right_Grip_Axis")),

		 // MixedReality mapping
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftSelect, FKey("MixedReality_Left_Trigger_Click")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftGrab, FKey("MixedReality_Left_Grip_Click")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightSelect, FKey("MixedReality_Right_Trigger_Click")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightGrab, FKey("MixedReality_Right_Grip_Click")),

		 // Oculus Touch mappings
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftSelect, FKey("OculusTouch_Left_Trigger_Click")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftGrab, FKey("OculusTouch_Left_Grip_Click")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightSelect, FKey("OculusTouch_Right_Trigger_Click")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightGrab, FKey("OculusTouch_Right_Grip_Click")),

		 // XRInputSimulation mapping
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftSelect, FKey("XRSimulation_Left_Select")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::LeftGrab, FKey("XRSimulation_Left_Grip")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightSelect, FKey("XRSimulation_Right_Select")),
		 FInputActionKeyMapping(UxtHandTrackerInputActions::RightGrab, FKey("XRSimulation_Right_Grip"))});

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

bool FUxtDefaultHandTracker::IsHandController(EControllerHand Hand) const
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
