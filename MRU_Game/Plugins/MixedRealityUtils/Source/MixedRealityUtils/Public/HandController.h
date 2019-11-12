// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HandController.generated.h"

class AMixedRealityHand;

UINTERFACE(BlueprintType)
class UHandController : public UInterface
{
    GENERATED_BODY()

};

/**
 * Interface for controlling hand movements and interactions.
 */
class MIXEDREALITYUTILS_API IHandController
{
    GENERATED_BODY()

public:

	static void UpdateHand(UObject *controller, AMixedRealityHand *hand);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsTracked(EControllerHand Hand) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsPinched(EControllerHand Hand) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FTransform GetRootTransform(EControllerHand Hand) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FVector GetTouchLocation(EControllerHand Hand) const;

};
