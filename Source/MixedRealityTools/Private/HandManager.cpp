// Fill out your copyright notice in the Description page of Project Settings.

#include "HandManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UHandManager::UHandManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

bool UHandManager::UpdateHandActor(EControllerHand Handedness)
{
	if (IHandController::Execute_IsTracked(Controller.GetObject(), Handedness))
	{
		APawn *pawn = Cast<APawn>(GetOwner());
		if (pawn)
		{
			APlayerController *controller = Cast<APlayerController>(pawn->GetController());
			if (controller)
			{
				SpawnHand(Handedness, controller);
				return true;
			}
		}
	}

	DestroyHand(Handedness);
	return false;
}

void UHandManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateHandActor(EControllerHand::Left);
	UpdateHandActor(EControllerHand::Right);

	if (Controller.GetObject())
	{
		for (auto item : Hands)
		{
			IHandController::UpdateHand(Controller.GetObject(), item.Value);
		}
	}
}

void UHandManager::SpawnHand(EControllerHand Handedness, APlayerController *PlayerController)
{
	if (Hands.Contains(Handedness))
	{
		return;
	}

	AMixedRealityHand *hand = GetWorld()->SpawnActor<AMixedRealityHand>(FVector::ZeroVector, FRotator::ZeroRotator);
	Hands.Add(Handedness, hand);

	hand->SetOwner(GetOwner());
	hand->Handedness = Handedness;

	hand->AttachToComponent(PlayerController->PlayerCameraManager->GetTransformComponent(), FAttachmentTransformRules::KeepRelativeTransform);
}

void UHandManager::DestroyHand(EControllerHand Handedness)
{
	AMixedRealityHand *hand;
	if (Hands.RemoveAndCopyValue(Handedness, hand))
	{
		hand->Destroy();
	}
}
