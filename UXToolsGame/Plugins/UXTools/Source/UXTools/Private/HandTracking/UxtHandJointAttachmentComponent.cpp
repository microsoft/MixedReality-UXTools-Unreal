// Fill out your copyright notice in the Description page of Project Settings.


#include "HandTracking/UxtHandJointAttachmentComponent.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UXTools.h"


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
	FQuat JointOrientation;
	FVector IndexTipPosition;
	FVector ThumbTipPosition;
	float JointRadius;

	if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexTip, JointOrientation, IndexTipPosition, JointRadius) &&
		UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::ThumbTip, JointOrientation, ThumbTipPosition, JointRadius))
	{
		const float Distance = (IndexTipPosition - ThumbTipPosition).Size();
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
			FVector PalmPosition;
			if (UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::Palm, JointOrientation, PalmPosition, JointRadius))
			{
				bIsGrasped = true;
				JointTransformInPalm = GetOwner()->GetTransform().GetRelativeTransform(FTransform(JointOrientation, PalmPosition));
				OnHandGraspStarted.Broadcast(this);
			}
		}
	}
}

void UUxtHandJointAttachmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	FQuat JointOrientation;
	FVector JointPosition;
	FTransform JointTransform;

	float JointRadius;
	bool bIsTracked;

	if (bIsGrasped)
	{
		bIsTracked = UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::Palm, JointOrientation, JointPosition, JointRadius);
		JointTransform = JointTransformInPalm * FTransform(JointOrientation, JointPosition);
	}
	else
	{
		bIsTracked = UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, Joint, JointOrientation, JointPosition, JointRadius);
		JointTransform = FTransform(JointOrientation, JointPosition);
	}

	if (bIsTracked)
	{
		// Enable actor
		Owner->SetActorHiddenInGame(false);
		Owner->SetActorEnableCollision(true);

		FVector Location = JointTransform.GetLocation();
		FQuat Rotation = JointTransform.GetRotation();

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
