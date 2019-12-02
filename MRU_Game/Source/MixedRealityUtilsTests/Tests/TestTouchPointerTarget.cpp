// Fill out your copyright notice in the Description page of Project Settings.

#include "TestTouchPointerTarget.h"

void UTestTouchPointerTarget::BeginPlay()
{
	Super::BeginPlay();

	TouchStartedCount = 0;
	TouchEndedCount = 0;
}

void UTestTouchPointerTarget::TouchStarted_Implementation(UTouchPointer* Pointer)
{
	++TouchStartedCount;
}

void UTestTouchPointerTarget::TouchEnded_Implementation(UTouchPointer* Pointer)
{
	++TouchEndedCount;
}

bool UTestTouchPointerTarget::GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface)
{
	return false;
}
