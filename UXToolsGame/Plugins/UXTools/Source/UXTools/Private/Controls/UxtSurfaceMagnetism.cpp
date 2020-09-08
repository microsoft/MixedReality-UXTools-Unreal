// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtSurfaceMagnetism.h"

#include "DrawDebugHelpers.h"

#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

// Sets default values for this component's properties
UUxtSurfaceMagnetism::UUxtSurfaceMagnetism()
{
	MagnetismType = EUxtMagnetismType::Head;
	TraceDistance = 2000.0f;
	bAutoBounds = false;
	BoxBounds = FVector(32.0f, 32.0f, 32.0f);
	CollisionProfile = TEXT("UI");
	TraceChannel = ECC_Visibility;
	bSmoothRotation = bSmoothRotation = false;
	PositionInterpValue = RotationInterpValue = 8.0f;
	ImpactNormalOffset = 0.0f;
	TraceRayOffset = 0.0f;
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	bOnlyYawEnabled = false;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UUxtSurfaceMagnetism::SetCollisionProfile(FName Profile)
{
	CollisionProfile = Profile;
	SetCollisionProfileName(CollisionProfile);
}

#if WITH_EDITORONLY_DATA
void UUxtSurfaceMagnetism::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SetupBounds();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UUxtSurfaceMagnetism::SetAutoBounds(bool DoAutoBounds)
{
	bAutoBounds = DoAutoBounds;
	SetupBounds();
}

void UUxtSurfaceMagnetism::SetBoxBounds(FVector NewBounds)
{
	BoxBounds = NewBounds;
	SetupBounds();
}

void UUxtSurfaceMagnetism::SetOnlyYawEnabled(bool UseYawOnly)
{
	bOnlyYawEnabled = UseYawOnly;
}

// Called every frame
void UUxtSurfaceMagnetism::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (FarPointerWeak.Get())
	{
		if (MagnetismType == EUxtMagnetismType::Head)
		{
			FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(GetWorld());
			FVector Start = HeadTransform.GetLocation();
			FVector End = Start + (HeadTransform.GetRotation().GetForwardVector() * TraceDistance);
			TraceAndSetActorLocation(Start, End, DeltaTime);
		}
		else if (MagnetismType == EUxtMagnetismType::Hand)
		{
			// account for tracking loss in target hand
			if (UUxtFarPointerComponent* FarPointer = FarPointerWeak.Get())
			{
				FVector Start = FarPointer->GetPointerOrigin();
				FVector End = Start + (FarPointer->GetPointerOrientation().GetForwardVector() * TraceDistance);
				TraceAndSetActorLocation(Start, End, DeltaTime);
			}
		}
	}
	else // interpolation once dropped
	{
		TraceAndSetActorLocation(FVector::ZeroVector, FVector::ZeroVector, DeltaTime); // Vector values unused
	}
}

void UUxtSurfaceMagnetism::OnComponentCreated()
{
	Super::OnComponentCreated();
	SetupBounds();
}

void UUxtSurfaceMagnetism::BeginPlay()
{
	Super::BeginPlay();
	SetCollisionProfileName(CollisionProfile);
}

bool UUxtSurfaceMagnetism::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return this == Primitive;
}

void UUxtSurfaceMagnetism::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer)
	{
		Pointer->SetFocusLocked(true);
		FarPointerWeak = Pointer;
		SetComponentTickEnabled(true);
		OnMagnetismStarted.Broadcast(this);
		bInteractionHalted = false;
	}
}

void UUxtSurfaceMagnetism::OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (Pointer)
	{
		Pointer->SetFocusLocked(false);
		FarPointerWeak = nullptr;
		if (bSmoothPosition || bSmoothRotation)
		{
			bInteractionHalted = true;
		}
		else
		{
			SetComponentTickEnabled(false);
			OnMagnetismEnded.Broadcast(this);
		}
	}
}

void UUxtSurfaceMagnetism::SetupBounds()
{
	if (bAutoBounds)
	{
		if (AActor* MyActor = GetOwner())
		{
			FBox Box = UUxtMathUtilsFunctionLibrary::CalculateNestedActorBoundsInLocalSpace(GetOwner(), true);
			BoxBounds = (Box.Max - Box.Min) / 2.0f;
			SetBoxExtent(BoxBounds);
		}
	}
	else
	{
		SetBoxExtent(BoxBounds);
	}
}

void UUxtSurfaceMagnetism::TraceAndSetActorLocation(FVector Start, FVector End, float DeltaTime)
{
	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.bTraceComplex = false;
		QueryParams.AddIgnoredActor(GetOwner());
		if (AActor* MyActor = GetOwner())
		{
			if (FarPointerWeak.Get() &&
				World->LineTraceSingleByChannel(
					Hit, Start, End, TraceChannel, QueryParams)) // if far pointer isn't valid we are just interping to target position
			{
				HitLocation = Hit.Location + (ImpactNormalOffset * Hit.ImpactNormal);
				HitRotation = UKismetMathLibrary::MakeRotFromX(Hit.ImpactNormal);
				if (bOnlyYawEnabled)
				{
					HitRotation.Pitch = HitRotation.Roll = 0.0f;
				}

				if (TraceRayOffset != 0.0f) // check to avoid doing a normalise when we don't need it
				{
					FVector Offset = Start - End;
					Offset.Normalize();
					Offset *= TraceRayOffset;
					HitLocation += Offset;
				}
			}

			MyActor->SetActorLocationAndRotation(
				bSmoothPosition ? FMath::VInterpTo(MyActor->GetActorLocation(), HitLocation, DeltaTime, PositionInterpValue) : HitLocation,
				bSmoothRotation ? FMath::RInterpTo(MyActor->GetActorRotation(), HitRotation, DeltaTime, RotationInterpValue) : HitRotation);

			if (bInteractionHalted)
			{
				const float DistanceThresholdSquare = 1.0f;
				const float RotationThreshold = 0.95f;

				bool DistanceThresholdMet = FVector::DistSquared(MyActor->GetActorLocation(), HitLocation) < DistanceThresholdSquare;
				bool RotationThresholdMet =
					FVector::DotProduct(MyActor->GetActorForwardVector(), HitRotation.Quaternion().GetForwardVector()) > RotationThreshold;

				if (DistanceThresholdMet && RotationThresholdMet)
				{
					SetComponentTickEnabled(false);
					OnMagnetismEnded.Broadcast(this);
				}
			}
		}
	}
}

bool UUxtSurfaceMagnetism::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return this == Primitive;
}