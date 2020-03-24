// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFarBeamComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "UXTools.h"


UUxtFarBeamComponent::UUxtFarBeamComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Will enable tick on far pointer activation
	PrimaryComponentTick.bStartWithTickEnabled = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/UXTools/Pointers/SM_Beam"));
	check(MeshFinder.Object);
	SetStaticMesh(MeshFinder.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/UXTools/Pointers/M_Beam"));
	check(MaterialFinder.Object);
	SetMaterial(0, MaterialFinder.Object);

	SetCastShadow(false);
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetHiddenInGame(true);

	// Set a reasonable default beam diameter
	SetRelativeScale3D(FVector(1.0f, 0.1f, 0.1f));
}

void UUxtFarBeamComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UUxtFarPointerComponent* FarPointer = GetOwner()->FindComponentByClass<UUxtFarPointerComponent>())
	{
		FarPointerWeak = FarPointer;

		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(FarPointer);

		// Activate now if the pointer is enabled
		if (FarPointer->IsEnabled())
		{
			OnFarPointerEnabled(FarPointer);
		}

		// Subscribe to pointer state changes
		FarPointer->OnFarPointerEnabled.AddDynamic(this, &UUxtFarBeamComponent::OnFarPointerEnabled);
		FarPointer->OnFarPointerDisabled.AddDynamic(this, &UUxtFarBeamComponent::OnFarPointerDisabled);
	}
	else
	{
		UE_LOG(UXTools, Error, TEXT("Could not find a far pointer in actor '%s'. Far beam won't work properly."), *GetOwner()->GetName());
	}
}

void UUxtFarBeamComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		FarPointer->OnFarPointerEnabled.RemoveDynamic(this, &UUxtFarBeamComponent::OnFarPointerEnabled);
		FarPointer->OnFarPointerDisabled.RemoveDynamic(this, &UUxtFarBeamComponent::OnFarPointerDisabled);
	}
}

void UUxtFarBeamComponent::OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer)
{
	SetActive(true);
	SetHiddenInGame(false);
}

void UUxtFarBeamComponent::OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer)
{
	SetActive(false);
	SetHiddenInGame(true);
}

void UUxtFarBeamComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		const FVector Start = FarPointer->GetRayStart();
		const FVector End = FarPointer->GetHitPoint() + FarPointer->GetHitNormal() * HoverDistance;

		// This direction may be different from the one given by the pointer orientation if the pointer is locked,
		// as the hit point will be locked to the hit primitive instead of located somewhere along the pointer ray.
		FVector Dir = End - Start;

		// Scale beam along X with the beam length. Keep Y and Z scales untouched.
		const float Length = Dir.Size();
		Dir = Dir / Length;
		FVector Scale = GetComponentScale();
		Scale.X = Length;

		// Calculate our rotation by rotating the pointer one so its forward vector is aligned with the beam direction.
		FQuat PointerOrientation = FarPointer->GetPointerOrientation();
		FQuat Rotation = FQuat::FindBetweenNormals(PointerOrientation.GetForwardVector(), Dir) * PointerOrientation;

		SetWorldTransform(FTransform(Rotation, Start, Scale));
	}
}
