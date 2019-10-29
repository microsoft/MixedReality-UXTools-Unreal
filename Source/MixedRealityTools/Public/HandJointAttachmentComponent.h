// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "WindowsMixedRealityHandTrackingTypes.h"
#include "HandJointAttachmentComponent.generated.h"


/**
 * Attaches an actor to a joint in the given hand, updating the actor translation and rotation to match the joint ones every frame.
 * The actor will be hidden and its collision disabled during hand tracking loss.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXEDREALITYTOOLS_API UHandJointAttachmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UHandJointAttachmentComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EControllerHand Hand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWMRHandKeypoint Joint;
};
