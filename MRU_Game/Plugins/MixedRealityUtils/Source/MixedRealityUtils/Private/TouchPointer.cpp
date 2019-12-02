// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchPointer.h"

#include "TouchPointerTarget.h"

// Sets default values for this component's properties
UTouchPointer::UTouchPointer()
{
	// No ticking needed for pointers.
	PrimaryComponentTick.bCanEverTick = false;

	// Create a collision sphere for detecting interactables
	TouchSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TouchSphere"));
	TouchSphere->InitSphereRadius(TouchRadius);
	TouchSphere->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	TouchSphere->SetCollisionProfileName(TEXT("OverlapAll"));
	TouchSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TouchSphere->SetGenerateOverlapEvents(true);
}

void UTouchPointer::OnPointerBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	TryStartTouching(OtherActor);
}

void UTouchPointer::OnPointerEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	TryStopTouching(OtherActor);
}

bool UTouchPointer::TryStartTouching(AActor* actor)
{
	bool result = false;
	const auto &components = actor->GetComponents();
	for (UActorComponent *comp : components)
	{
		if (ImplementsTargetInterface(comp))
		{
			ITouchPointerTarget::Execute_TouchStarted(comp, this);
			TouchedTargets.Add(comp);
			result = true;
		}
	}
	return result;
}

bool UTouchPointer::TryStopTouching(AActor* actor)
{
	bool result = false;
	const auto &components = actor->GetComponents();
	for (UActorComponent *comp : components)
	{
		if (TouchedTargets.Remove(comp) > 0)
		{
			if (ImplementsTargetInterface(comp))
			{
				ITouchPointerTarget::Execute_TouchEnded(comp, this);
			}

			result = true;
		}
	}
	return result;
}

void UTouchPointer::StopAllTouching()
{
	for (UActorComponent* Target : TouchedTargets)
	{
		if (Target)
		{
			ITouchPointerTarget::Execute_TouchEnded(Target, this);
		}
	}
	TouchedTargets.Empty();
}

bool UTouchPointer::ImplementsTargetInterface(const UObject *obj) const
{
	return obj->GetClass()->ImplementsInterface(UTouchPointerTarget::StaticClass()) || Cast<ITouchPointerTarget>(obj);
}

void UTouchPointer::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->OnActorBeginOverlap.AddDynamic(this, &UTouchPointer::OnPointerBeginOverlap);
	GetOwner()->OnActorEndOverlap.AddDynamic(this, &UTouchPointer::OnPointerEndOverlap);
}

void UTouchPointer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllTouching();

	GetOwner()->OnActorBeginOverlap.RemoveDynamic(this, &UTouchPointer::OnPointerBeginOverlap);
	GetOwner()->OnActorEndOverlap.RemoveDynamic(this, &UTouchPointer::OnPointerEndOverlap);

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

UActorComponent* UTouchPointer::GetClosestPointOnTargets(FVector& OutPointOnTargetSurface) const
{
	UActorComponent* ClosestTarget = nullptr;
	float DistanceSqr = MAX_flt;
	const auto PointerPosition = GetComponentLocation();

	// Iterate over all current targets obtaining their closest point to the pointer.
	for (UActorComponent* TargetComponent : TouchedTargets)
	{
		if (TargetComponent)
		{
			FVector ClosestPoint;
			if (ITouchPointerTarget::Execute_GetClosestPointOnSurface(TargetComponent, PointerPosition, ClosestPoint))
			{
				auto NewDistanceSqr = FVector::DistSquared(PointerPosition, ClosestPoint);
				if (NewDistanceSqr < DistanceSqr)
				{
					DistanceSqr = NewDistanceSqr;
					ClosestTarget = TargetComponent;
					OutPointOnTargetSurface = ClosestPoint;
				}
			}
		}
	}

	return ClosestTarget;
}

bool UTouchPointer::GetGrasped() const
{
	return bIsGrasped;
}

void UTouchPointer::SetGrasped(bool Enable)
{
	if (bIsGrasped != Enable)
	{
		if (Enable)
		{
			OnBeginPinch.Broadcast(this);
		}
		else
		{
			OnEndPinch.Broadcast(this);
		}
		bIsGrasped = Enable;
	}
}
