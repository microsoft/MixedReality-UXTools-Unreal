// Fill out your copyright notice in the Description page of Project Settings.


#include "HandTracking/UxtHandJointAttachmentComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UXTools.h"

#define UXT_WMR (PLATFORM_WINDOWS || PLATFORM_HOLOLENS)

#if UXT_WMR
#include "WindowsMixedRealityHandTrackingTypes.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"
#endif

namespace
{
	bool GetModifiedHandJointTransform(EControllerHand Hand, EUxtHandKeypoint Keypoint, FTransform& OutTransform, float& OutRadius)
	{
#if UXT_WMR
		// We need to rotate the hand joint transforms here so that they comply with UE standards.
		// After rotating these transforms, if you have your hand flat on a table, palm down, the 
		// positive x of each joint should point away from the wrist and the positive z should
		// point away from the table.

		EWMRHandKeypoint KeypointWMR = (EWMRHandKeypoint)Keypoint;
		bool success = UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, KeypointWMR, OutTransform, OutRadius);
		OutTransform.SetRotation(OutTransform.GetRotation() * FQuat(FVector::RightVector, PI));
		return success;
#else
		return false;
#endif
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

#if UXT_WMR
	if (bAttachOnSkin)
	{
		if (!LocalAttachDirection.Normalize())
		{
			UE_LOG(UXTools, Error, TEXT("Could not normalize LocalAttachDirection. The calculated attachment position won't be on the skin"));
		}
	}
#endif // UXT_WMR
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
#if UXT_WMR
	FTransform IndexTipTransform;
	FTransform ThumbTipTransform;
	float JointRadius;

	if (GetModifiedHandJointTransform(Hand, EUxtHandKeypoint::IndexTip, IndexTipTransform, JointRadius) &&
		GetModifiedHandJointTransform(Hand, EUxtHandKeypoint::ThumbTip, ThumbTipTransform, JointRadius))
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
			if (GetModifiedHandJointTransform(Hand, EUxtHandKeypoint::Palm, PalmTransform, JointRadius))
			{
				bIsGrasped = true;
				JointTransformInPalm = GetOwner()->GetTransform().GetRelativeTransform(PalmTransform);
				OnHandGraspStarted.Broadcast(this);
			}
		}
	}
#endif
}

void UUxtHandJointAttachmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if UXT_WMR
	AActor* Owner = GetOwner();
	FTransform Transform;
	float JointRadius;
	bool bIsTracked;

	if (bIsGrasped)
	{
		bIsTracked = GetModifiedHandJointTransform(Hand, EUxtHandKeypoint::Palm, Transform, JointRadius);
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
#endif // UXT_WMR
}
