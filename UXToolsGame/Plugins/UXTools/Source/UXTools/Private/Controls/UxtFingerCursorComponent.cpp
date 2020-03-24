// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFingerCursorComponent.h"
#include "Input/UxtTouchPointer.h"
#include "UXTools.h"
#include "GameFramework/Actor.h"


UUxtFingerCursorComponent::UUxtFingerCursorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	RingThickness = 0.3f;
	BorderThickness = 0.02f;

	// We want the ring to remain a constant thickness regardless of the radius
	bUseAbsoluteThickness = true;

	// Remain hidden until we see a valid touch target
	SetHiddenInGame(true);
}

void UUxtFingerCursorComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	UUxtTouchPointer* TouchPointer = Owner->FindComponentByClass<UUxtTouchPointer>();
	TouchPointerWeak = TouchPointer;

	if (TouchPointer)
	{
		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(TouchPointer);
	}
	else
	{
		UE_LOG(UXTools, Error, TEXT("Could not find a touch pointer in actor '%s'. Finger cursor won't work properly."), *Owner->GetName());
	}
}

void UUxtFingerCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtTouchPointer* TouchPointer = TouchPointerWeak.Get())
	{
		FVector PointOnTarget;
		if (auto Target = TouchPointer->GetHoveredTarget(PointOnTarget))
		{
			const auto PointerToTarget = PointOnTarget - TouchPointer->GetComponentLocation();
			const auto DistanceToTarget = PointerToTarget.Size();

			// Must use an epsilon to avoid unreliable rotations as we get closer to the target
			const float Epsilon = 0.000001;

			if (DistanceToTarget > Epsilon)
			{
				FTransform IndexTipTransform = TouchPointer->GetComponentTransform();

				if (DistanceToTarget < AlignWithSurfaceDistance)
				{
					// Slerp between surface normal and index tip rotation
					float slerpAmount = DistanceToTarget / AlignWithSurfaceDistance;
					SetWorldRotation(FQuat::Slerp(PointerToTarget.ToOrientationQuat(), IndexTipTransform.GetRotation(), slerpAmount));
				}
				else
				{
					SetWorldRotation(IndexTipTransform.GetRotation());
				}
			}

			// Scale radius with the distance to the target
			float Alpha = DistanceToTarget / MaxDistanceToTarget;
			float NewRadius = FMath::Lerp(MinRadius, MaxRadius, Alpha);
			SetRadius(NewRadius);

			if (bHiddenInGame)
			{
				SetHiddenInGame(false);
			}
		}
		else if(!bHiddenInGame)
		{
			// Hide mesh when the pointer has no target
			SetHiddenInGame(true);
		}
	}
}
