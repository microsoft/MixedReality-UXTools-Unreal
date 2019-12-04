// Fill out your copyright notice in the Description page of Project Settings.


#include "HandJointAttachmentComponent.h"
#include "Kismet\GameplayStatics.h"
#include "MixedRealityToolsFunctionLibrary.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"
#include "MixedRealityUtils.h"


UHandJointAttachmentComponent::UHandJointAttachmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick before physics as the tick could affect the transform of simulated actors.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UHandJointAttachmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!UMixedRealityToolsFunctionLibrary::IsHandTrackingAvailable())
	{
		AActor* Owner = GetOwner();

		// Attach to player camera
		if (APlayerCameraManager* Manager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
		{
			FVector Location;

			if (Hand == EControllerHand::Left)
			{
				Location.Set(30, -10, 0);
			}
			else
			{
				Location.Set(30, 10, 0);
			}

			Owner->SetActorLocation(Location);
			Owner->AttachToActor(Manager, FAttachmentTransformRules::KeepRelativeTransform);
		}

		// Bind to LMB to simulate grasp
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			// Enable input
			Owner->EnableInput(PlayerController);

			if (UInputComponent* InputComponent = Owner->FindComponentByClass<UInputComponent>())
			{
				FInputChord InputChord(EKeys::LeftMouseButton);

				// Use modifier keys to discern between left and right
				if (Hand == EControllerHand::Left)
				{
					InputChord.bShift = true;
				}
				else
				{
					InputChord.bAlt = true;
				}
				
				// Bind to LMB press and release
				InputComponent->BindKey(InputChord, EInputEvent::IE_Pressed, this, &UHandJointAttachmentComponent::OnLmbPressed);
				InputComponent->BindKey(InputChord, EInputEvent::IE_Released, this, &UHandJointAttachmentComponent::OnLmbReleased);
			}
		}

		SetComponentTickEnabled(false);
	}
	else if (bAttachOnSkin)
	{
		if (!LocalAttachDirection.Normalize())
		{
			UE_LOG(MixedRealityUtils, Error, TEXT("Could not normalize LocalAttachDirection. The calculated attachment position won't be on the skin"));
		}
	}
}

void UHandJointAttachmentComponent::OnLmbPressed()
{
	if (!bIsGrasped)
	{
		bIsGrasped = true;
		OnHandGraspStarted.Broadcast(this);
	}
}

void UHandJointAttachmentComponent::OnLmbReleased()
{
	if (bIsGrasped)
	{
		bIsGrasped = false;
		OnHandGraspEnded.Broadcast(this);
	}
}

void UHandJointAttachmentComponent::UpdateGraspState()
{
	FTransform IndexTipTransform;
	FTransform ThumbTipTransform;
	float JointRadius;

	if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, EWMRHandKeypoint::IndexTip, IndexTipTransform, JointRadius) &&
		UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, EWMRHandKeypoint::ThumbTip, ThumbTipTransform, JointRadius))
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
			if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, EWMRHandKeypoint::Palm, PalmTransform, JointRadius))
			{
				bIsGrasped = true;
				JointTransformInPalm = GetOwner()->GetTransform().GetRelativeTransform(PalmTransform);
				OnHandGraspStarted.Broadcast(this);
			}
		}
	}
}

void UHandJointAttachmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	FTransform Transform;
	float JointRadius;
	bool bIsTracked;

	if (bIsGrasped)
	{
		bIsTracked = UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, EWMRHandKeypoint::Palm, Transform, JointRadius);
		Transform = JointTransformInPalm * Transform;
	}
	else
	{
		bIsTracked = UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Joint, Transform, JointRadius);
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
