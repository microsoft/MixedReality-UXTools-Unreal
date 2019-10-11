// Fill out your copyright notice in the Description page of Project Settings.

#include "TouchPointer.h"

#include "TouchPointerTarget.h"

// Sets default values for this component's properties
UTouchPointer::UTouchPointer()
{
    // No ticking needed for pointers.
    PrimaryComponentTick.bCanEverTick = false;

    // Set to make sure InitializeComponent gets called.
    bWantsInitializeComponent = true;

    // Create a collision sphere for detecting interactables
    m_touchSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TouchSphere"));
    m_touchSphere->InitSphereRadius(TouchRadius);
    m_touchSphere->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
    m_touchSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    m_touchSphere->SetGenerateOverlapEvents(true);
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

void UTouchPointer::InitializeComponent()
{
	// Create event delegates for handling overlap begin/end
	m_beginOverlapDelegate.BindUFunction(this, "OnActorBeginOverlap");
	m_endOverlapDelegate.BindUFunction(this, "OnActorEndOverlap");
}

void UTouchPointer::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->OnActorBeginOverlap.AddUnique(m_beginOverlapDelegate);
	GetOwner()->OnActorEndOverlap.AddUnique(m_endOverlapDelegate);
}

void UTouchPointer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllTouching();

	GetOwner()->OnActorBeginOverlap.Remove(m_beginOverlapDelegate);
	GetOwner()->OnActorEndOverlap.Remove(m_endOverlapDelegate);

	Super::EndPlay(EndPlayReason);
}

void UTouchPointer::Activate(bool bReset)
{
	Super::Activate();

	GetOwner()->OnActorBeginOverlap.AddUnique(m_beginOverlapDelegate);
	GetOwner()->OnActorEndOverlap.AddUnique(m_endOverlapDelegate);
}

void UTouchPointer::Deactivate()
{
	StopAllTouching();

	GetOwner()->OnActorBeginOverlap.Remove(m_beginOverlapDelegate);
	GetOwner()->OnActorEndOverlap.Remove(m_endOverlapDelegate);

	Super::Deactivate();
}

void UTouchPointer::SetTouchRadius(float radius)
{
    this->TouchRadius = radius;
    m_touchSphere->SetSphereRadius(radius);
}
