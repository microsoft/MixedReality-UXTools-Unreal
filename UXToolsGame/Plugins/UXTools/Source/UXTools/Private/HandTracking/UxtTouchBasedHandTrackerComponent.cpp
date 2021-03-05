// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtTouchBasedHandTrackerComponent.h"

#include "Features/IModularFeatures.h"
#include "GameFramework/PlayerController.h"
#include "Utils/UxtFunctionLibrary.h"

void UUxtTouchBasedHandTrackerComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerController = Cast<APlayerController>(GetOwner());
	check(PlayerController);

	// Replace the existing hand tracker. The WMR hand tracker is registered on UXT plugin load When running in mobile preview.
	OldHandTracker = &IUxtHandTracker::Get();
	if (OldHandTracker)
	{
		IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), OldHandTracker);
	}

	IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), this);
}

void UUxtTouchBasedHandTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), this);
	if (OldHandTracker)
	{
		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), OldHandTracker);
	}

	Super::EndPlay(EndPlayReason);
}

ETrackingStatus UUxtTouchBasedHandTrackerComponent::GetTrackingStatus(EControllerHand Hand) const
{
	return IsTouchPressed(Hand) ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;
}

bool UUxtTouchBasedHandTrackerComponent::IsHandController(EControllerHand Hand) const
{
	// Touch input is always hand input
	return true;
}

bool UUxtTouchBasedHandTrackerComponent::GetJointState(
	EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	OutRadius = 0;
	bool bTracked = GetPointerPose(Hand, OutOrientation, OutPosition);

	if (bTracked && Joint == EHandKeypoint::Palm)
	{
		// Use head orientation as the palm's one as it is used to check if the hand is in pointing pose in
		// AUxtHandInteractionActor::IsInPointingPose
		OutOrientation = UUxtFunctionLibrary::GetHeadPose(PlayerController).GetRotation();
	}

	return bTracked;
}

bool UUxtTouchBasedHandTrackerComponent::GetFingerState(EControllerHand Hand, float& OutScreenX, float& OutScreenY) const
{
	ETouchIndex::Type FingerIndex = Hand == EControllerHand::Left ? ETouchIndex::Touch1 : ETouchIndex::Touch2;
	bool bIsCurrentlyPressed;
	PlayerController->GetInputTouchState(FingerIndex, OutScreenX, OutScreenY, bIsCurrentlyPressed);
	return bIsCurrentlyPressed;
}

bool UUxtTouchBasedHandTrackerComponent::IsTouchPressed(EControllerHand Hand) const
{
	float ScreenX, ScreenY;
	return GetFingerState(Hand, ScreenX, ScreenY);
}

bool UUxtTouchBasedHandTrackerComponent::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	float ScreenX;
	float ScreenY;

	if (GetFingerState(Hand, ScreenX, ScreenY))
	{
		FVector WorldLocation;
		FVector WorldDirection;

		if (PlayerController->DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldLocation, WorldDirection))
		{
			OutOrientation = FQuat(FRotationMatrix::MakeFromX(WorldDirection));
			OutPosition = WorldLocation;
			return true;
		}
	}

	return false;
}

bool UUxtTouchBasedHandTrackerComponent::GetGripPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	return GetPointerPose(Hand, OutOrientation, OutPosition);
}

bool UUxtTouchBasedHandTrackerComponent::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	OutIsGrabbing = IsTouchPressed(Hand) ? true : OutIsGrabbing;
	return true;
}

bool UUxtTouchBasedHandTrackerComponent::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	OutIsSelectPressed = IsTouchPressed(Hand) ? true : OutIsSelectPressed;
	return true;
}
