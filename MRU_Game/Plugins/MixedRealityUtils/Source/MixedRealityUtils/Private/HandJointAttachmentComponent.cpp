// Fill out your copyright notice in the Description page of Project Settings.


#include "HandJointAttachmentComponent.h"
#include "Kismet\GameplayStatics.h"
#include "MixedRealityToolsFunctionLibrary.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"
#include "MixedRealityUtils.h"


UHandJointAttachmentComponent::UHandJointAttachmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UHandJointAttachmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!UMixedRealityToolsFunctionLibrary::IsHandTrackingAvailable())
	{
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

			GetOwner()->SetActorLocation(Location);
			GetOwner()->AttachToActor(Manager, FAttachmentTransformRules::KeepRelativeTransform);
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

void UHandJointAttachmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	FTransform Transform;
	float JointRadius;

	if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Joint, Transform, JointRadius))
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
	}
	else
	{
		// Disable actor on hand tracking loss
		Owner->SetActorHiddenInGame(true);
		Owner->SetActorEnableCollision(false);
	}
}
