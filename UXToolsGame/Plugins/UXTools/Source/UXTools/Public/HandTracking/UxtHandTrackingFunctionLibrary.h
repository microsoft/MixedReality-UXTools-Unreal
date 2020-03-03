// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HandTracking/IUxtHandTracker.h"
#include "UxtHandTrackingFunctionLibrary.generated.h"


/**
 * Library of hand tracking functions for UX Tools.
 */
UCLASS(ClassGroup = UXTools)
class UXTOOLS_API UUxtHandTrackingFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Obtain the state of the given joint. Returns false if the hand is not currently tracked, in which case the values of the output parameters are unchanged. */
	UFUNCTION(BlueprintCallable, Category = "HandTracking|UXTools")
	static bool GetHandJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius);

	/** Obtain the pointer pose. Returns false if the hand is not tracked this frame, in which case the value of the output parameter is unchanged. */
	UFUNCTION(BlueprintCallable, Category = "HandTracking|UXTools")
	static bool GetHandPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition);

	/** Obtain current grabbed state. Returns false if the hand is not currently tracked, in which case the value of the output parameter is unchanged. */
	UFUNCTION(BlueprintCallable, Category = "HandTracking|UXTools")
	static bool GetIsHandGrabbing(EControllerHand Hand, bool& OutIsGrabbing);

	/** Returns whether the given hand is tracked. */
	UFUNCTION(BlueprintCallable, Category = "HandTracking|UXTools")
	static bool IsHandTracked(EControllerHand Hand);

	/** Returns the current registered hand tracker or nullptr if none */
	static IUxtHandTracker* GetHandTracker();
};
