// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtDefaultHandTracker.h"

#include "Features/IModularFeatures.h"
#include "GameFramework/InputSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogUxtDefaultHandTracker, Log, All);

namespace
{
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

void FUxtDefaultHandTracker::RegisterEnhancedInputAction(UInputMappingContext* InputContext, UInputAction*& Action, FText Description, TArray<FKey> Keys)
{
	Action = NewObject<UInputAction>();
	Action->AddToRoot();

	Action->ActionDescription = Description;
	Action->Triggers.Add(NewObject<UInputTriggerDown>());
	Action->bConsumeInput = false;

	for (FKey Key : Keys)
	{
		InputContext->MapKey(Action, Key);
	}
}

void FUxtDefaultHandTracker::RegisterInputMappings(UInputMappingContext* InputContext)
{
	RegisterEnhancedInputAction(InputContext, LeftSelect, 
		FText::FromName(TEXT("UxtLeftSelect")), TArray<FKey> {
		FKey("OpenXRMsftHandInteraction_Left_Select_Axis"),
			FKey("MixedReality_Left_Trigger_Click"),
			FKey("OculusTouch_Left_Trigger_Click"),
			FKey("XRSimulation_Left_Select")
	});
	RegisterEnhancedInputAction(InputContext, LeftGrab,
		FText::FromName(TEXT("UxtLeftGrab")), TArray<FKey> {
		FKey("OpenXRMsftHandInteraction_Left_Grip_Axis"),
			FKey("MixedReality_Left_Grip_Click"),
			FKey("OculusTouch_Left_Grip_Click"),
			FKey("XRSimulation_Left_Grip")
	});

	RegisterEnhancedInputAction(InputContext, RightSelect,
		FText::FromName(TEXT("UxtRightSelect")), TArray<FKey> {
		FKey("OpenXRMsftHandInteraction_Right_Select_Axis"),
			FKey("MixedReality_Right_Trigger_Click"),
			FKey("OculusTouch_Right_Trigger_Click"),
			FKey("XRSimulation_Right_Select")
	});
	RegisterEnhancedInputAction(InputContext, RightGrab,
		FText::FromName(TEXT("UxtRightGrab")), TArray<FKey> {
		FKey("OpenXRMsftHandInteraction_Right_Grip_Axis"),
			FKey("MixedReality_Right_Grip_Click"),
			FKey("OculusTouch_Right_Grip_Click"),
			FKey("XRSimulation_Right_Grip")
	});
}

void FUxtDefaultHandTracker::UnregisterInputMappings(UInputMappingContext* InputContext)
{
	InputContext->UnmapAllKeysFromAction(LeftSelect);
	InputContext->UnmapAllKeysFromAction(LeftGrab);
	InputContext->UnmapAllKeysFromAction(RightSelect);
	InputContext->UnmapAllKeysFromAction(RightGrab);
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
