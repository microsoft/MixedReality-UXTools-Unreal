// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchPointer.h"

#include "TouchPointerTarget.h"

TArray<UTouchPointer*> UTouchPointer::Pointers;

// Sets default values for this component's properties
UTouchPointer::UTouchPointer()
{
    // No ticking needed for pointers.
    PrimaryComponentTick.bCanEverTick = false;
}

void UTouchPointer::OnActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	const auto &components = OtherActor->GetComponents();
	for (auto *comp : components)
	{
		TryStartTouching(comp);
	}
}

void UTouchPointer::OnActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	const auto &components = OtherActor->GetComponents();
	for (auto *comp : components)
	{
		StopTouching(comp);
    }
}

bool UTouchPointer::TryStartTouching(UActorComponent *comp)
{
	if (ImplementsTargetInterface(comp))
	{
		ITouchPointerTarget::Execute_TouchStarted(comp, this);
		TouchedTargets.Add(comp);
		return true;
	}
	return false;
}

bool UTouchPointer::StopTouching(UActorComponent *comp)
{
	if (ImplementsTargetInterface(comp))
	{
		ITouchPointerTarget::Execute_TouchEnded(comp, this);
	}

	return TouchedTargets.Remove(comp) > 0;
}

void UTouchPointer::StopAllTouching()
{
	for (const TWeakObjectPtr<UActorComponent>& wComp : TouchedTargets)
	{
		if (UActorComponent *comp = wComp.Get())
		{
			ITouchPointerTarget::Execute_TouchEnded(comp, this);
		}
	}
	TouchedTargets.Empty();
}

bool UTouchPointer::ImplementsTargetInterface(const UActorComponent *comp) const
{
	return comp->GetClass()->ImplementsInterface(UTouchPointerTarget::StaticClass()) || Cast<ITouchPointerTarget>(comp);
}

void UTouchPointer::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->OnActorBeginOverlap.AddDynamic(this, &UTouchPointer::OnActorBeginOverlap);
	GetOwner()->OnActorEndOverlap.AddDynamic(this, &UTouchPointer::OnActorEndOverlap);

	Pointers.Add(this);
}

void UTouchPointer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllTouching();

	Pointers.Remove(this);

	GetOwner()->OnActorBeginOverlap.RemoveDynamic(this, &UTouchPointer::OnActorBeginOverlap);
	GetOwner()->OnActorEndOverlap.RemoveDynamic(this, &UTouchPointer::OnActorEndOverlap);

	Super::EndPlay(EndPlayReason);
}

const TArray<UTouchPointer*>& UTouchPointer::GetAllPointers()
{
	return Pointers;
}
