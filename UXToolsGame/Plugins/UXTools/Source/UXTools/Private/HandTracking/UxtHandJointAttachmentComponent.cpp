// Fill out your copyright notice in the Description page of Project Settings.


#include "HandTracking/UxtHandJointAttachmentComponent.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UXTools.h"

namespace
{
	bool GetModifiedHandJointTransform(EControllerHand Hand, EUxtHandJoint Keypoint, FTransform& OutTransform, float& OutRadius)
	{
		FQuat Orientation;
		FVector Position;

		if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, Keypoint, Orientation, Position, OutRadius))
		{
			OutTransform.SetTranslationAndScale3D(Position, FVector::OneVector);

			// We need to rotate the hand joint transforms here so that they comply with UE standards.
			// After rotating these transforms, if you have your hand flat on a table, palm down, the 
			// positive x of each joint should point away from the wrist and the positive z should
			// point away from the table.
			OutTransform.SetRotation(Orientation * FQuat(FVector::RightVector, PI));

			return true;
		}

		return false;
	}
}

UUxtHandJointAttachmentComponent::UUxtHandJointAttachmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick before physics as the tick could affect the transform of simulated actors.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UUxtHandJointAttachmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAttachOnSkin)
	{
		if (!LocalAttachDirection.Normalize())
		{
			UE_LOG(UXTools, Error, TEXT("Could not normalize LocalAttachDirection. The calculated attachment position won't be on the skin"));
		}
	}
}

void UUxtHandJointAttachmentComponent::OnLmbPressed()
{
	if (!bIsGrasped)
	{
		bIsGrasped = true;
		OnHandGraspStarted.Broadcast(this);
	}
}

void UUxtHandJointAttachmentComponent::OnLmbReleased()
{
	if (bIsGrasped)
	{
		bIsGrasped = false;
		OnHandGraspEnded.Broadcast(this);
	}
}

void UUxtHandJointAttachmentComponent::UpdateGraspState()
{
	FTransform IndexTipTransform;
	FTransform ThumbTipTransform;
	float JointRadius;

	if (GetModifiedHandJointTransform(Hand, EUxtHandJoint::IndexTip, IndexTipTransform, JointRadius) &&
		GetModifiedHandJointTransform(Hand, EUxtHandJoint::ThumbTip, ThumbTipTransform, JointRadius))
	{
		const float Distance = (IndexTipTransform.GetTranslation() - ThumbTipTransform.GetTranslation()).Size();
		const float GraspStartDistance = 2;
		const float GraspEndDistance = 4.5;

		if (bIsGrasped)
		{
			if (Distance > GraspEndDistance)
			{
				bIsGrasped = false;
				OnHandGraspEnded.Broadcast(this);
			}
		}
		else if (Distance <= GraspStartDistance)
		{
			FTransform PalmTransform;
			if (GetModifiedHandJointTransform(Hand, EUxtHandJoint::Palm, PalmTransform, JointRadius))
			{
				bIsGrasped = true;
				JointTransformInPalm = GetOwner()->GetTransform().GetRelativeTransform(PalmTransform);
				OnHandGraspStarted.Broadcast(this);
			}
		}
	}
}

void UUxtHandJointAttachmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	FTransform Transform;
	float JointRadius;
	bool bIsTracked;

	if (bIsGrasped)
	{
		bIsTracked = GetModifiedHandJointTransform(Hand, EUxtHandJoint::Palm, Transform, JointRadius);
		Transform = JointTransformInPalm * Transform;
	}
	else
	{
		bIsTracked = GetModifiedHandJointTransform(Hand, Joint, Transform, JointRadius);
	}

	if (bIsTracked)
	{
		// Enable actor
		Owner->SetActorHiddenInGame(false);
		Owner->SetActorEnableCollision(true);

		FVector Location = Transform.GetLocation();
		FQuat Rotation = Transform.GetRotation();

		if (bAttachOnSkin)
		{
			Location += Rotation.RotateVector(LocalAttachDirection) * JointRadius;
		}

		// Update transform
		Owner->SetActorLocationAndRotation(Location, Rotation);

		UpdateGraspState();
	}
	else
	{
		if (bIsGrasped)
		{
			bIsGrasped = false;
			OnHandGraspEnded.Broadcast(this);
		}

		// Disable actor on hand tracking loss
		Owner->SetActorHiddenInGame(true);
		Owner->SetActorEnableCollision(false);
	}
}
