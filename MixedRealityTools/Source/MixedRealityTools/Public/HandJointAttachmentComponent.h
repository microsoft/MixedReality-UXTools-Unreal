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

	/** Hand to attach to (left or right). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EControllerHand Hand;

	/** Joint to attach to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWMRHandKeypoint Joint;

	/** If this is set the attachment point will be on the skin surface near the joint instead of the joint itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAttachOnSkin;

	/** 
	 * When attaching to the skin, this direction is used to compute the attachment point from the joint position.
	 * -X indicates forward in the direction of the bone, +Y right and +Z down (towards the palm).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAttachOnSkin"))
	FVector LocalAttachDirection = { -1, 0, 0 };
};
