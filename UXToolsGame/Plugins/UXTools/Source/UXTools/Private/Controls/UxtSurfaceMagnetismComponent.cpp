// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtSurfaceMagnetismComponent.h"

#include "DrawDebugHelpers.h"

#include "ARTraceResult.h"
#include "ARBlueprintLibrary.h"
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

	if (!GetTargetComponent() && GetOwner())
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
		float HitDistance = std::numeric_limits<float>::max();
		bool HasHit = false;
		{
			FHitResult Hit;
			FCollisionQueryParams QueryParams;
			QueryParams.bTraceComplex = false;
			QueryParams.AddIgnoredComponent(Target);

			if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, QueryParams))
			{
				TargetLocation = Hit.Location + (ImpactNormalOffset * Hit.ImpactNormal);
				TargetRotation = UKismetMathLibrary::MakeRotFromX(Hit.ImpactNormal);

				HitDistance = Hit.Distance;
				HasHit = true;
			}
		}

		// UE-169219: In 5.1 Unreal has switched over to their chaos physics engine, which is not currently working with the MRMesh.
		// LineTraceTrackedObjects3D will hit test against any tracked objects like the MRMesh without using the physics engine.
		TArray<FARTraceResult> HitTestSR = UARBlueprintLibrary::LineTraceTrackedObjects3D(Start, End, true, true, true, true);
		if (!HitTestSR.IsEmpty())
		{
			FARTraceResult Hit = HitTestSR[0];

			FVector HitLocation = Hit.GetLocalTransform().GetLocation();
			FQuat HitRotation = Hit.GetLocalTransform().GetRotation();

			// Check if this hit against the spatial map is closer than the hit against the physics mesh.
			if (!HasHit || (HasHit && FVector::Distance(Start, HitLocation) < HitDistance))
			{
				TargetLocation = HitLocation + (ImpactNormalOffset * HitRotation.GetForwardVector());
				TargetRotation = FRotator(HitRotation);
			}

			HasHit = true;
		}

		if (HasHit)
		{
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
