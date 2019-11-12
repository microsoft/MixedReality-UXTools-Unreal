// Fill out your copyright notice in the Description page of Project Settings.

#include "MixedRealityHand.h"
#include "Components/InputComponent.h"
#include "DrawDebugHelpers.h"

#include "TouchPointer.h"
#include "HandController.h"

AMixedRealityHand::AMixedRealityHand()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a root component to locate the hand
	auto root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(root);

	// Create a touch pointer for interactions
	TouchPointer = CreateDefaultSubobject<UTouchPointer>(TEXT("TouchPointer"));
	TouchPointer->SetupAttachment(root);
}

void AMixedRealityHand::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DebugDraw();
}

void AMixedRealityHand::SetTouchPointerWorldLocation(FVector location)
{
	TouchPointer->SetWorldLocation(location);
}

void AMixedRealityHand::SetTouchPointerRelativeLocation(FVector location)
{
	TouchPointer->SetRelativeLocation(location);
}

void AMixedRealityHand::DebugDraw()
{
	UWorld *world = GetWorld();
	FVector center = GetActorLocation();

	float radius = 0.8f;

	DrawDebugSphere(world, center, radius, 16, GetColor());

	DrawDebugSolidBox(world, TouchPointer->GetComponentLocation(), FVector(1, 1, 1), FColor::Cyan);
}

FColor AMixedRealityHand::GetColor() const
{
	switch (Handedness)
	{
	case EControllerHand::Left:
		return FColor::Red;
	case EControllerHand::Right:
		return FColor::Blue;
	}
	return FColor::Magenta;
}
