// Fill out your copyright notice in the Description page of Project Settings.


#include "HandJointAttachmentComponent.h"
#include "Kismet\GameplayStatics.h"
#include "MixedRealityToolsFunctionLibrary.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"


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
}

void UHandJointAttachmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	FTransform Transform;

	if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Joint, Transform))
	{
		// Enable actor
		Owner->SetActorHiddenInGame(false);
		Owner->SetActorEnableCollision(true);

		// Update transform
		Owner->SetActorLocationAndRotation(Transform.GetLocation(), Transform.GetRotation());
	}
	else
	{
		// Disable actor on hand tracking loss
		Owner->SetActorHiddenInGame(true);
		Owner->SetActorEnableCollision(false);
	}
}
