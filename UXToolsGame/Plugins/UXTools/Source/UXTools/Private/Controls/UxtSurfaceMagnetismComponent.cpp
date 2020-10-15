// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtSurfaceMagnetismComponent.h"

#include "DrawDebugHelpers.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

// Sets default values for this component's properties
UUxtSurfaceMagnetismComponent::UUxtSurfaceMagnetismComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = false;
}

UPrimitiveComponent* UUxtSurfaceMagnetismComponent::GetTargetComponent() const
{
	return Cast<UPrimitiveComponent>(TargetComponent.GetComponent(GetOwner()));
}

void UUxtSurfaceMagnetismComponent::SetTargetComponent(UPrimitiveComponent* Target)
{
	TargetComponent.OverrideComponent = Target;
}

// Called every frame
void UUxtSurfaceMagnetismComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// account for tracking loss in target hand
	if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
	{
		FVector Start = FarPointer->GetPointerOrigin();
		FVector End = Start + (FarPointer->GetPointerOrientation().GetForwardVector() * TraceDistance);
		TraceAndSetActorLocation(Start, End, DeltaTime);
	}
}

void UUxtSurfaceMagnetismComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetTargetComponent())
	{
		SetTargetComponent(Cast<UPrimitiveComponent>(GetOwner()->GetComponentByClass(UPrimitiveComponent::StaticClass())));
	}
}

bool UUxtSurfaceMagnetismComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return Primitive == GetTargetComponent();
}

void UUxtSurfaceMagnetismComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && !FarPointerWeak.IsValid())
	{
		Pointer->SetFocusLocked(true);
		FarPointerWeak = Pointer;
		SetActive(true);
		OnMagnetismStarted.Broadcast(this);

		// Initialize target with current transform
		if (UPrimitiveComponent* Target = GetTargetComponent())
		{
			TargetLocation = Target->GetComponentLocation();
			TargetRotation = Target->GetComponentRotation();
		}
	}
}

void UUxtSurfaceMagnetismComponent::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer && Pointer == FarPointerWeak.Get())
	{
		Pointer->SetFocusLocked(false);
		FarPointerWeak = nullptr;
		SetActive(false);
		OnMagnetismEnded.Broadcast(this);
	}
}

void UUxtSurfaceMagnetismComponent::TraceAndSetActorLocation(FVector Start, FVector End, float DeltaTime)
{
	if (UPrimitiveComponent* Target = GetTargetComponent())
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.bTraceComplex = false;
		QueryParams.AddIgnoredComponent(Target);

		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, QueryParams))
		{
			TargetLocation = Hit.Location + (ImpactNormalOffset * Hit.ImpactNormal);
			TargetRotation = UKismetMathLibrary::MakeRotFromX(Hit.ImpactNormal);
			if (bKeepOrientationVertical)
			{
				TargetRotation.Pitch = TargetRotation.Roll = 0.0f;
			}

			if (TraceRayOffset != 0.0f) // check to avoid doing a normalise when we don't need it
			{
				FVector Offset = Start - End;
				Offset.Normalize();
				Offset *= TraceRayOffset;
				TargetLocation += Offset;
			}
		}

		Target->SetWorldLocationAndRotation(
			bSmoothPosition ? FMath::VInterpTo(Target->GetComponentLocation(), TargetLocation, DeltaTime, PositionInterpValue)
							: TargetLocation,
			bSmoothRotation ? FMath::RInterpTo(Target->GetComponentRotation(), TargetRotation, DeltaTime, RotationInterpValue)
							: TargetRotation);
	}
}

bool UUxtSurfaceMagnetismComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return Primitive == GetTargetComponent();
}
