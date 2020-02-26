// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HandTracking/IUxtHandTracker.h"
#include "InputCoreTypes.h"
#include "UxtHandJointAttachmentComponent.generated.h"

// TODO Remove these once we have a unified API to access joint transforms and hand "button" states.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtHandGraspStartedDelegate, UUxtHandJointAttachmentComponent*, HandJointAttachment);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtHandGraspEndedDelegate, UUxtHandJointAttachmentComponent*, HandJointAttachment);

/**
 * Attaches an actor to a joint in the given hand, updating the actor translation and rotation to match the joint ones every frame.
 * The actor will be hidden and its collision disabled during hand tracking loss.
 */
UCLASS( ClassGroup = UXTools, meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtHandJointAttachmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UUxtHandJointAttachmentComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

	void UpdateGraspState();

private:

	// TODO Remove these when we have hand simulation at platform level.

	/** Simulate grasp start on LMB press + shift or alt when running in editor */
	void OnLmbPressed();

	/** Simulate grasp end on LMB release + shift or alt when running in editor */
	void OnLmbReleased();

public:

	/** Hand to attach to (left or right). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Joint Attachment")
	EControllerHand Hand;

	/** Joint to attach to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Joint Attachment")
	EUxtHandJoint Joint;

	/** If this is set the attachment point will be on the skin surface near the joint instead of the joint itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Joint Attachment")
	bool bAttachOnSkin;

	/** 
	 * When attaching to the skin, this direction is used to compute the attachment point from the joint position.
	 * +X indicates forward in the direction of the bone, +Y right and +Z up (away from the palm).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Joint Attachment", meta = (EditCondition = "bAttachOnSkin"))
	FVector LocalAttachDirection = { 1, 0, 0 };

	/** If true, raise grasp events and use the joint transform relative to the palm while grasping to increase stability */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Joint Attachment")
	bool bHandleGrasp = true;

	UPROPERTY(BlueprintAssignable, Category = "Hand Joint Attachment")
	FUxtHandGraspStartedDelegate OnHandGraspStarted;

	UPROPERTY(BlueprintAssignable, Category = "Hand Joint Attachment")
	FUxtHandGraspEndedDelegate OnHandGraspEnded;

private:

	/** Joint transform in palm space. Captured on grasp start. */
	FTransform JointTransformInPalm;

	bool bIsGrasped = false;
};
