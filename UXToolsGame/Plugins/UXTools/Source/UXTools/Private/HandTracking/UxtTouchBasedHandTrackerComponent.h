// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

#include "Components/ActorComponent.h"
#include "HandTracking/IUxtHandTracker.h"

#include "UxtTouchBasedHandTrackerComponent.generated.h"

class UWorld;
class APlayerController;

/**
 * Component added automatically by UXT to the player controller to enable driving far interactions via touch input.
 * The hand tracker interface is used just to provide the pointer pose and grab/select states, GetJointState() returns
 * that pointer pose for all joints.
 */
UCLASS(ClassGroup = "UXTools|Internal")
class UXTOOLS_API UUxtTouchBasedHandTrackerComponent
	: public UActorComponent
	, public IUxtHandTracker
{
	GENERATED_BODY()

public:
	//
	// IUxtHandTracker interface

	virtual bool GetJointState(
		EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const;
	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const;
	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const;
	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const;

	// UActorComponent
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool GetFingerState(EControllerHand Hand, float& OutScreenX, float& OutScreenY) const;

	UPROPERTY(Transient)
	APlayerController* PlayerController;

	IUxtHandTracker* OldHandTracker = nullptr;
};
