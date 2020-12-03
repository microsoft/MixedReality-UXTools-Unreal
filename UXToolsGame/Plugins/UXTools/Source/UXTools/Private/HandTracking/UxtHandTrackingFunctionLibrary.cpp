// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

#include "IXRTrackingSystem.h"

#include "Features/IModularFeatures.h"

#include "GameFramework/InputSettings.h"

ETrackingStatus UUxtHandTrackingFunctionLibrary::GetTrackingStatus(EControllerHand Hand)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetTrackingStatus(Hand);
	}

	return ETrackingStatus::NotTracked;
}

bool UUxtHandTrackingFunctionLibrary::HasHandData(EControllerHand Hand)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->HasHandData(Hand);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetHandJointState(
	EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetJointState(Hand, Joint, OutOrientation, OutPosition, OutRadius);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetHandPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetPointerPose(Hand, OutOrientation, OutPosition);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetGripPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetGripPose(Hand, OutOrientation, OutPosition);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(EControllerHand Hand, bool& OutIsGrabbing)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetIsGrabbing(Hand, OutIsGrabbing);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed)
{
	if (IUxtHandTracker* HandTracker = IUxtHandTracker::GetHandTracker())
	{
		return HandTracker->GetIsSelectPressed(Hand, OutIsSelectPressed);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::IsHandTracked(EControllerHand Hand)
{
	bool NotUsed = false;
	return GetIsHandGrabbing(Hand, NotUsed);
}

void UUxtHandTrackingFunctionLibrary::DebugActions()
{
	const UInputSettings* InputSettings = GetDefault<UInputSettings>();
	if (InputSettings != nullptr)
	{
		TArray<FName> ActionNames;
		InputSettings->GetActionNames(ActionNames);

		for (const FName& ActionName : ActionNames)
		{
			UE_LOG(LogTemp, Display, TEXT("OOOOOOOO: %s"), *ActionName.ToString());
		}
	}
}

/** Temporary hack to expose valid WMR motion controller data to blueprints for use with XRVisualization. */
bool UUxtHandTrackingFunctionLibrary::GetMotionControllerData(UObject* World, EControllerHand Hand, FXRMotionControllerData& OutMotionControllerData)
{
	if (IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get())
	{
		XRSystem->GetMotionControllerData(World, Hand, OutMotionControllerData);

		// XXX HACK: WMR always reporting invalid hand data, remove once fixed!
		OutMotionControllerData.bValid = true;
		OutMotionControllerData.DeviceVisualType =
			OutMotionControllerData.HandKeyPositions.Num() == EHandKeypointCount ? EXRVisualType::Hand
			: EXRVisualType::Controller;
		OutMotionControllerData.GripPosition = OutMotionControllerData.AimPosition;
		// XXX

		return true;
	}

	return false;
}
