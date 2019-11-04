// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchPointer.h"

#include "TouchPointerTarget.h"

TArray<UTouchPointer*> UTouchPointer::Pointers;

// Sets default values for this component's properties
UTouchPointer::UTouchPointer()
{
	// No ticking needed for pointers.
	PrimaryComponentTick.bCanEverTick = false;

	// Create a collision sphere for detecting interactables
	TouchSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TouchSphere"));
	TouchSphere->InitSphereRadius(TouchRadius);
	TouchSphere->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
	TouchSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TouchSphere->SetGenerateOverlapEvents(true);
}

void UTouchPointer::OnPointerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	TryStartTouching(OtherComp);
}

void UTouchPointer::OnPointerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	TryStopTouching(OtherComp);
}

bool UTouchPointer::TryStartTouching(USceneComponent *comp)
{
	while (comp)
	{
		if (ImplementsTargetInterface(comp))
		{
			ITouchPointerTarget::Execute_TouchStarted(comp, this);
			TouchedTargets.Add((UActorComponent*)comp);

			if (bIsPinched)
			{
				ITouchPointerTarget::Execute_PinchStarted(comp, this);
			}

			return true;
		}

		comp = comp->GetAttachParent();
	}
	return false;
}

bool UTouchPointer::TryStopTouching(USceneComponent *comp)
{
	while (comp)
	{
		if (TouchedTargets.Remove((UActorComponent*)comp) > 0)
		{
			if (ImplementsTargetInterface(comp))
			{
				if (bIsPinched)
				{
					ITouchPointerTarget::Execute_PinchEnded(comp, this);
				}

				ITouchPointerTarget::Execute_TouchEnded(comp, this);
			}

			return true;
		}

		comp = comp->GetAttachParent();
	}
	return false;
}

void UTouchPointer::StopAllTouching()
{
	for (UActorComponent* Target : TouchedTargets)
	{
		if (Target)
		{
			if (bIsPinched)
			{
				ITouchPointerTarget::Execute_PinchEnded(Target, this);
			}

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

	TouchSphere->OnComponentBeginOverlap.AddDynamic(this, &UTouchPointer::OnPointerBeginOverlap);
	TouchSphere->OnComponentEndOverlap.AddDynamic(this, &UTouchPointer::OnPointerEndOverlap);

	Pointers.Add(this);
}

void UTouchPointer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllTouching();

	Pointers.Remove(this);

	TouchSphere->OnComponentBeginOverlap.RemoveDynamic(this, &UTouchPointer::OnPointerBeginOverlap);
	TouchSphere->OnComponentEndOverlap.RemoveDynamic(this, &UTouchPointer::OnPointerEndOverlap);

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

const TArray<UTouchPointer*>& UTouchPointer::GetAllPointers()
{
	return Pointers;
}

bool UTouchPointer::GetPinched() const
{
	return bIsPinched;
}

void UTouchPointer::SetPinched(bool Enable)
{
	if (bIsPinched != Enable)
	{
		for (const TWeakObjectPtr<UActorComponent>& wComp : TouchedTargets)
		{
			if (UActorComponent *comp = wComp.Get())
			{
				if (Enable)
				{
					ITouchPointerTarget::Execute_PinchStarted(comp, this);
				}
				{
					ITouchPointerTarget::Execute_PinchEnded(comp, this);
				}
			}
		}

		bIsPinched = Enable;
	}
}
