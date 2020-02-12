// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationHeadMovementComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "UXToolsInputSimulation"

void UxtInputSimulationHeadMovementComponent::AddRotationInput(const FRotator& Rotation)
{
	RotationInput += Rotation;
}

void UxtInputSimulationHeadMovementComponent::AddMovementInput(const FVector& Movement)
{
	MovementInput += Movement;
}

// Copied from APlayerCameraManager::ProcessViewRotation
void UxtInputSimulationHeadMovementComponent::ApplyRotationInput(float DeltaTime)
{
	// Calculate Delta to be applied on rotation
	FRotator DeltaRot(RotationInput);

	// Add Delta Rotation
	ViewOrientation += DeltaRot;
	DeltaRot = FRotator::ZeroRotator;

	const float ViewPitchMin = -89.9f;
	const float ViewPitchMax = 89.9f;
	const float ViewYawMin = 0.f;
	const float ViewYawMax = 359.999f;
	const float ViewRollMin = -89.9f;
	const float ViewRollMax = 89.9f;

	ViewOrientation.Pitch = FMath::ClampAngle(ViewOrientation.Pitch, ViewPitchMin, ViewPitchMax);
	ViewOrientation.Pitch = FRotator::ClampAxis(ViewOrientation.Pitch);

	ViewOrientation.Roll = FMath::ClampAngle(ViewOrientation.Roll, ViewRollMin, ViewRollMax);
	ViewOrientation.Roll = FRotator::ClampAxis(ViewOrientation.Roll);

	ViewOrientation.Yaw = FMath::ClampAngle(ViewOrientation.Yaw, ViewYawMin, ViewYawMax);
	ViewOrientation.Yaw = FRotator::ClampAxis(ViewOrientation.Yaw);
}

// Copied from UFloatingPawnMovement
void UxtInputSimulationHeadMovementComponent::ApplyMovementInput(float DeltaTime)
{
	const float MaxSpeed = 1200.f;
	const float Acceleration = 4000.f;
	const float Deceleration = 8000.f;
	const float TurningBoost = 8.0f;

	const FVector ControlAcceleration = MovementInput.GetClampedToMaxSize(1.f);

	const float AnalogInputModifier = (ControlAcceleration.SizeSquared() > 0.f ? ControlAcceleration.Size() : 0.f);
	const float MaxPawnSpeed = MaxSpeed * AnalogInputModifier;
	const bool bExceedingMaxSpeed = IsExceedingMaxSpeed(MaxPawnSpeed);

	if (AnalogInputModifier > 0.f && !bExceedingMaxSpeed)
	{
		// Apply change in velocity direction
		if (Velocity.SizeSquared() > 0.f)
		{
			// Change direction faster than only using acceleration, but never increase velocity magnitude.
			const float TimeScale = FMath::Clamp(DeltaTime * TurningBoost, 0.f, 1.f);
			Velocity = Velocity + (ControlAcceleration * Velocity.Size() - Velocity) * TimeScale;
		}
	}
	else
	{
		// Dampen velocity magnitude based on deceleration.
		if (Velocity.SizeSquared() > 0.f)
		{
			const FVector OldVelocity = Velocity;
			const float VelSize = FMath::Max(Velocity.Size() - FMath::Abs(Deceleration) * DeltaTime, 0.f);
			Velocity = Velocity.GetSafeNormal() * VelSize;

			// Don't allow braking to lower us below max speed if we started above it.
			if (bExceedingMaxSpeed && Velocity.SizeSquared() < FMath::Square(MaxPawnSpeed))
			{
				Velocity = OldVelocity.GetSafeNormal() * MaxPawnSpeed;
			}
		}
	}

	// Apply acceleration and clamp velocity magnitude.
	const float NewMaxSpeed = (IsExceedingMaxSpeed(MaxPawnSpeed)) ? Velocity.Size() : MaxPawnSpeed;
	Velocity += ControlAcceleration * FMath::Abs(Acceleration) * DeltaTime;
	Velocity = Velocity.GetClampedToMaxSize(NewMaxSpeed);
}

void UxtInputSimulationHeadMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	ViewOrientation = FRotator::ZeroRotator;
	ViewPosition = FVector::ZeroVector;
	RotationInput = FRotator::ZeroRotator;
	MovementInput = FVector::ZeroVector;
}

void UxtInputSimulationHeadMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!UpdatedComponent || ShouldSkipUpdate(DeltaTime))
	{
		return;
	}

	// Apply accumulated axis input
	ApplyRotationInput(DeltaTime);
	ApplyMovementInput(DeltaTime);
	// Reset axis input
	RotationInput = FRotator::ZeroRotator;
	MovementInput = FVector::ZeroVector;

	// Rotate view
	UpdatedComponent->SetWorldRotation(ViewOrientation);

	// Move actor
	FVector Delta = Velocity * DeltaTime;
	if (!Delta.IsNearlyZero(1e-6f))
	{
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FQuat Rotation = UpdatedComponent->GetComponentQuat();

		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);

		if (Hit.IsValidBlockingHit())
		{
			HandleImpact(Hit, DeltaTime, Delta);
			// Try to slide the remaining distance along the surface.
			SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		}

		// Update velocity
		const FVector NewLocation = UpdatedComponent->GetComponentLocation();
		Velocity = ((NewLocation - OldLocation) / DeltaTime);
	}

	// Finalize
	UpdateComponentVelocity();
}

#undef LOCTEXT_NAMESPACE
