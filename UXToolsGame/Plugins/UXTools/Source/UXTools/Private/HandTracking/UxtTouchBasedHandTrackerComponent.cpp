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
	OldHandTracker = IUxtHandTracker::GetHandTracker();
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

bool UUxtTouchBasedHandTrackerComponent::GetJointState(
	EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	OutRadius = 0;
	bool bTracked = GetPointerPose(Hand, OutOrientation, OutPosition);

	if (bTracked && Joint == EUxtHandJoint::Palm)
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

bool UUxtTouchBasedHandTrackerComponent::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	float ScreenX;
	float ScreenY;
	OutIsGrabbing = GetFingerState(Hand, ScreenX, ScreenY) ? true : OutIsGrabbing;
	return OutIsGrabbing;
}

bool UUxtTouchBasedHandTrackerComponent::GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const
{
	float ScreenX;
	float ScreenY;
	OutIsSelectPressed = GetFingerState(Hand, ScreenX, ScreenY) ? true : OutIsSelectPressed;
	return OutIsSelectPressed;
}
