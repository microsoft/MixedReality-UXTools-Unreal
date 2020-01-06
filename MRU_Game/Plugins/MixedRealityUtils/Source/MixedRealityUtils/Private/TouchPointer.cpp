// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchPointer.h"

#include "TouchPointerTarget.h"


namespace
{
	bool IsTouchTarget(const UObject* Object)
	{
		// Cast<ITouchPointerTarget>(Object) doesn't work for BPs that implement the interface. This works for both C++ and BP implementers.
		return Object->GetClass()->ImplementsInterface(UTouchPointerTarget::StaticClass());
	}
}

// Sets default values for this component's properties
UTouchPointer::UTouchPointer()
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

void UTouchPointer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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
			if (IsTouchTarget(Component) && ITouchPointerTarget::Execute_GetClosestPointOnSurface(Component, PointerLocation, PointOnTarget))
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

void UTouchPointer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ChangeHoveredTarget(nullptr, FVector::ZeroVector);

	Super::EndPlay(EndPlayReason);
}

void UTouchPointer::SetTouchRadius(float radius)
{
	this->TouchRadius = radius;
	TouchSphere->SetSphereRadius(radius);
}

float UTouchPointer::GetTouchRadius() const 
{
	return TouchRadius;
}

UActorComponent* UTouchPointer::GetHoveredTarget(FVector& OutClosestPointOnTarget) const
{
	if (auto Target = HoveredTargetWeak.Get())
	{
		OutClosestPointOnTarget = ClosestPointOnHoveredTarget;
		return Target;
	}

	return nullptr;
}

bool UTouchPointer::SetHoveredTarget(UActorComponent* NewHoveredTarget, bool bEnableHoverLock)
{
	if (!bHoverLocked)
	{
		FVector PointerLocation = GetComponentLocation();
		FVector PointOnTarget;
		if (IsTouchTarget(NewHoveredTarget) && ITouchPointerTarget::Execute_GetClosestPointOnSurface(NewHoveredTarget, PointerLocation, PointOnTarget))
		{
			ChangeHoveredTarget(NewHoveredTarget, PointOnTarget);
		}

		bHoverLocked = (NewHoveredTarget != nullptr && bEnableHoverLock);

		return true;
	}
	return false;
}

void UTouchPointer::ChangeHoveredTarget(UActorComponent* NewHoveredTarget, const FVector& NewClosestPointOnTarget)
{
	auto HoveredTarget = HoveredTargetWeak.Get();

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
			ITouchPointerTarget::Execute_HoverEnded(HoveredTarget, this);
		}

		HoveredTargetWeak = NewHoveredTarget;
		ClosestPointOnHoveredTarget = NewClosestPointOnTarget;

		if (NewHoveredTarget)
		{
			ITouchPointerTarget::Execute_HoverStarted(NewHoveredTarget, this);
		}
	}
}

bool UTouchPointer::GetHoverLocked() const
{
	return bHoverLocked;
}

void UTouchPointer::SetHoverLocked(bool Value)
{
	bHoverLocked = Value;
}

bool UTouchPointer::GetGrasped() const
{
	return bIsGrasped;
}

void UTouchPointer::SetGrasped(bool bValue)
{
	if (bIsGrasped != bValue)
	{
		bIsGrasped = bValue;

		if (auto HoveredTarget = HoveredTargetWeak.Get())
		{
			if (bIsGrasped)
			{
				ITouchPointerTarget::Execute_GraspStarted(HoveredTarget, this);
			}
			else
			{
				ITouchPointerTarget::Execute_GraspEnded(HoveredTarget, this);
			}
		}
	}
}
