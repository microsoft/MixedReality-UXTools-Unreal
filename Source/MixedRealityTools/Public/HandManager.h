// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "HandController.h"
#include "MixedRealityHand.h"
#include "HandManager.generated.h"

class APlayerController;

/**
 * Component that manages hands in a pawn using a controller.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXEDREALITYTOOLS_API UHandManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHandManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TScriptInterface<UHandController> Controller;

	UFUNCTION(BlueprintCallable)
	bool UpdateHandActor(EControllerHand Handedness);

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void SpawnHand(EControllerHand Handedness, APlayerController *PlayerController);
	void DestroyHand(EControllerHand Handedness);

private:

	TMap<EControllerHand, AMixedRealityHand*> Hands;

};
