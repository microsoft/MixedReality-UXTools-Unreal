// Fill out your copyright notice in the Description page of Project Settings.

#include "TestTouchPointerTarget.h"

void UTestTouchPointerTarget::BeginPlay()
{
	Super::BeginPlay();

	TouchStartedCount = 0;
	TouchEndedCount = 0;
}

void UTestTouchPointerTarget::HoverStarted_Implementation(UTouchPointer* Pointer)
{
	++TouchStartedCount;
}

void UTestTouchPointerTarget::HoverEnded_Implementation(UTouchPointer* Pointer)
{
	++TouchEndedCount;
}

bool UTestTouchPointerTarget::GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface)
{
	OutPointOnSurface = GetComponentLocation();
	return true;
}
