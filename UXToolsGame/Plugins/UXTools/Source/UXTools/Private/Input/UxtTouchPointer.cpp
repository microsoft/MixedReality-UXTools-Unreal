// Fill out your copyright notice in the Description page of Project Settings.

#include "Input/UxtTouchPointer.h"

#include "Interactions/UxtTouchPointerTarget.h"


namespace
{
	bool IsTouchTarget(const UObject* Object)
	{
		// Cast<ITouchPointerTarget>(Object) doesn't work for BPs that implement the interface. This works for both C++ and BP implementers.
		return Object->GetClass()->ImplementsInterface(UUxtTouchPointerTarget::StaticClass());
	}
}

// Sets default values for this component's properties
UUxtTouchPointer::UUxtTouchPointer()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Tick after physics so overlaps reflect the latest physics state.
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	// Create a collision sphere for detecting interactables
	TouchSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TouchSphere"));
	TouchSphere->InitSphereRadius(TouchRadius);
	TouchSphere->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	TouchSphere->SetCollisionProfileName(TEXT("OverlapAll"));
	TouchSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TouchSphere->SetGenerateOverlapEvents(true);
}

void UUxtTouchPointer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bHoverLocked)
	{
		// Don't update the hovered target if we're locked
		return;
	}

	// Get overlapping actors
	TSet<AActor*> OverlappingActors;
	GetOwner()->GetOverlappingActors(OverlappingActors);

	const FVector PointerLocation = GetComponentLocation();
	float MinDistanceSqr = MAX_FLT;
	UActorComponent* ClosestTarget = nullptr;
	FVector ClosestPointOnTarget;

	// Find the closest touch target among all components in overlapping actors
	for (const AActor* OverlappingActor : OverlappingActors)
	{
		for (UActorComponent* Component : OverlappingActor->GetComponents())
		{
			FVector PointOnTarget;
			if (IsTouchTarget(Component) && IUxtTouchPointerTarget::Execute_GetClosestPointOnSurface(Component, PointerLocation, PointOnTarget))
			{
				float DistanceSqr = (PointerLocation - PointOnTarget).SizeSquared();
				if (DistanceSqr < MinDistanceSqr)
				{
					MinDistanceSqr = DistanceSqr;
					ClosestTarget = Component;
					ClosestPointOnTarget = PointOnTarget;
				}
			}
		}
	}

	ChangeHoveredTarget(ClosestTarget, ClosestPointOnTarget);
}

void UUxtTouchPointer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChangeHoveredTarget(nullptr, FVector::ZeroVector);

	Super::EndPlay(EndPlayReason);
}

void UUxtTouchPointer::SetTouchRadius(float radius)
{
	this->TouchRadius = radius;
	TouchSphere->SetSphereRadius(radius);
}

float UUxtTouchPointer::GetTouchRadius() const 
{
	return TouchRadius;
}

UObject* UUxtTouchPointer::GetHoveredTarget(FVector& OutClosestPointOnTarget) const
{
	if (auto Target = HoveredTargetWeak.Get())
	{
		OutClosestPointOnTarget = ClosestPointOnHoveredTarget;
		return Target;
	}

	return nullptr;
}

bool UUxtTouchPointer::SetHoveredTarget(UActorComponent* NewHoveredTarget, bool bEnableHoverLock)
{
	if (!bHoverLocked)
	{
		FVector PointerLocation = GetComponentLocation();
		FVector PointOnTarget;
		if (IsTouchTarget(NewHoveredTarget) && IUxtTouchPointerTarget::Execute_GetClosestPointOnSurface(NewHoveredTarget, PointerLocation, PointOnTarget))
		{
			ChangeHoveredTarget(NewHoveredTarget, PointOnTarget);
		}

		bHoverLocked = (NewHoveredTarget != nullptr && bEnableHoverLock);

		return true;
	}
	return false;
}

void UUxtTouchPointer::ChangeHoveredTarget(UActorComponent* NewHoveredTarget, const FVector& NewClosestPointOnTarget)
{
	UObject* HoveredTarget = HoveredTargetWeak.Get();

	// If hovered target is unchanged, then update only the closest-point-on-target
	if (NewHoveredTarget == HoveredTarget)
	{
		ClosestPointOnHoveredTarget = NewClosestPointOnTarget;
	}
	else
	{
		// Update hovered target
		if (HoveredTarget)
		{
			IUxtTouchPointerTarget::Execute_HoverEnded(HoveredTarget, this);
		}

		if (NewHoveredTarget)
		{
			HoveredTargetWeak = NewHoveredTarget;
			HoveredTarget = NewHoveredTarget;
		}
		else
		{
			// If the new target is null, use the default target instead.
			HoveredTargetWeak = DefaultTargetWeak;
			HoveredTarget = DefaultTargetWeak.Get();
		}
		ClosestPointOnHoveredTarget = NewClosestPointOnTarget;

		if (HoveredTarget)
		{
			IUxtTouchPointerTarget::Execute_HoverStarted(HoveredTarget, this);
		}
	}
}

bool UUxtTouchPointer::GetHoverLocked() const
{
	return bHoverLocked;
}

void UUxtTouchPointer::SetHoverLocked(bool Value)
{
	bHoverLocked = Value;
}

bool UUxtTouchPointer::GetGrasped() const
{
	return bIsGrasped;
}

void UUxtTouchPointer::SetGrasped(bool bValue)
{
	if (bIsGrasped != bValue)
	{
		bIsGrasped = bValue;

		if (bIsGrasped)
		{
			if (auto Target = HoveredTargetWeak.Get())
			{
				IUxtTouchPointerTarget::Execute_GraspStarted(Target, this);
			}
		}
		else
		{
			if (auto Target = HoveredTargetWeak.Get())
			{
				IUxtTouchPointerTarget::Execute_GraspEnded(Target, this);
			}
		}
	}
}

UObject* UUxtTouchPointer::GetDefaultTarget() const
{
	return DefaultTargetWeak.Get();
}

void UUxtTouchPointer::SetDefaultTarget(UObject* NewDefaultTarget)
{
	if (ensureMsgf(IsTouchTarget(NewDefaultTarget), TEXT("Target object must implement TouchPointerTarget interface")))
	{
		DefaultTargetWeak = NewDefaultTarget;
	}
}
