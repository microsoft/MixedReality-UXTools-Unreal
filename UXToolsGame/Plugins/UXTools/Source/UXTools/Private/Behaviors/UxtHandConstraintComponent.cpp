// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Behaviors/UxtHandConstraintComponent.h"

#include "Engine/World.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Utils/UxtFunctionLibrary.h"

namespace
{
	// Like FMath::LineBoxIntersection but from the inside and returns the result
	inline bool LineBoxIntersectionInternal(const FBox& Box, const FVector& Start, const FVector& End, FVector& HitLocation)
	{
		const FVector StartToEnd = End - Start;
		const FVector OneOverStartToEnd = StartToEnd.Reciprocal();

		FVector Time;

		if (Start.X >= Box.Min.X && Start.X <= Box.Max.X)
		{
			if (End.X < Box.Min.X)
			{
				Time.X = (Box.Min.X - Start.X) * OneOverStartToEnd.X;
			}
			else if (End.X > Box.Max.X)
			{
				Time.X = (Box.Max.X - Start.X) * OneOverStartToEnd.X;
			}
			else
			{
				Time.X = 2.0f;
			}
		}
		else
		{
			return false;
		}

		if (Start.Y >= Box.Min.Y && Start.Y <= Box.Max.Y)
		{
			if (End.Y < Box.Min.Y)
			{
				Time.Y = (Box.Min.Y - Start.Y) * OneOverStartToEnd.Y;
			}
			else if (End.Y > Box.Max.Y)
			{
				Time.Y = (Box.Max.Y - Start.Y) * OneOverStartToEnd.Y;
			}
			else
			{
				Time.Y = 2.0f;
			}
		}
		else
		{
			return false;
		}

		if (Start.Z >= Box.Min.Z && Start.Z <= Box.Max.Z)
		{
			if (End.Z < Box.Min.Z)
			{
				Time.Z = (Box.Min.Z - Start.Z) * OneOverStartToEnd.Z;
			}
			else if (End.Z > Box.Max.Z)
			{
				Time.Z = (Box.Max.Z - Start.Z) * OneOverStartToEnd.Z;
			}
			else
			{
				Time.Z = 2.0f;
			}
		}
		else
		{
			return false;
		}

		const float MinTime = FMath::Min3(Time.X, Time.Y, Time.Z);
		if (MinTime >= 0.0f && MinTime <= 1.0f)
		{
			HitLocation = Start + StartToEnd * MinTime;
			const float BOX_SIDE_THRESHOLD = 0.1f;
			if (HitLocation.X > Box.Min.X - BOX_SIDE_THRESHOLD && HitLocation.X < Box.Max.X + BOX_SIDE_THRESHOLD &&
				HitLocation.Y > Box.Min.Y - BOX_SIDE_THRESHOLD && HitLocation.Y < Box.Max.Y + BOX_SIDE_THRESHOLD &&
				HitLocation.Z > Box.Min.Z - BOX_SIDE_THRESHOLD && HitLocation.Z < Box.Max.Z + BOX_SIDE_THRESHOLD)
			{
				return true;
			}
		}

		return false;
	}

} // namespace

UUxtHandConstraintComponent::UUxtHandConstraintComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

EControllerHand UUxtHandConstraintComponent::GetTrackedHand() const
{
	return TrackedHand;
}

const FBox& UUxtHandConstraintComponent::GetHandBounds() const
{
	return HandBounds;
}

bool UUxtHandConstraintComponent::IsConstraintActive() const
{
	return bIsConstraintActive;
}

const FVector& UUxtHandConstraintComponent::GetGoalLocation() const
{
	return GoalLocation;
}

const FQuat& UUxtHandConstraintComponent::GetGoalRotation() const
{
	return GoalRotation;
}

bool UUxtHandConstraintComponent::IsHandUsableForConstraint(EControllerHand NewHand)
{
	// Accept by default
	return true;
}

void UUxtHandConstraintComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize tracked hand from user setting.
	// If Hand is 'Any Hand' the left is arbitrarily chosen, will switch to right if not tracked.
	TrackedHand = (Hand == EControllerHand::Left || Hand == EControllerHand::Right) ? Hand : EControllerHand::Left;

	bIsConstraintActive = false;
	UpdateConstraint();
}

void UUxtHandConstraintComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateConstraint();

	if (bIsConstraintActive && bMoveOwningActor)
	{
		AddMovement(DeltaTime);
	}
}

FVector UUxtHandConstraintComponent::GetZoneDirection(const FVector& HandLocation, const FQuat& HandRotation) const
{
	// Directions are for the left hand case
	FVector DirectionUlnar;
	FVector DirectionUp;
	switch (OffsetMode)
	{
	case EUxtHandConstraintOffsetMode::LookAtCamera:
	{
		FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());
		FVector LookAtVector = HandLocation - HeadPose.GetLocation();
		bool IsPalmFacingCamera = FVector::DotProduct(LookAtVector, HandRotation.GetUpVector()) > 0.0f;

		DirectionUlnar =
			!LookAtVector.IsNearlyZero() ? FVector::CrossProduct(LookAtVector, FVector::UpVector).GetSafeNormal() : -FVector::RightVector;
		if (IsPalmFacingCamera)
		{
			DirectionUlnar = -DirectionUlnar;
		}
		DirectionUp = HeadPose.GetRotation().GetUpVector();
		break;
	}

	case EUxtHandConstraintOffsetMode::HandRotation:
	{
		DirectionUlnar = -HandRotation.GetRightVector();
		DirectionUp = HandRotation.GetForwardVector();
		break;
	}
	}

	// Flip for the right hand case
	if (TrackedHand == EControllerHand::Right)
	{
		DirectionUlnar = -DirectionUlnar;
	}

	switch (Zone)
	{
	case EUxtHandConstraintZone::UlnarSide:
		return DirectionUlnar;
	case EUxtHandConstraintZone::RadialSide:
		return -DirectionUlnar;
	case EUxtHandConstraintZone::AboveFingerTips:
		return DirectionUp;
	case EUxtHandConstraintZone::BelowWrist:
		return -DirectionUp;
	}

	return FVector::ZeroVector;
}

void UUxtHandConstraintComponent::UpdateConstraint()
{
	const bool bWasConstraintActive = bIsConstraintActive;
	const EControllerHand OldTrackedHand = TrackedHand;

	bIsConstraintActive = false;
	FQuat PalmRotation;
	FVector PalmLocation;
	if (UpdateTrackedHand(PalmLocation, PalmRotation))
	{
		if (UpdateHandBounds(PalmLocation, PalmRotation))
		{
			if (UpdateGoal(PalmLocation, PalmRotation))
			{
				// Activate constraint
				bIsConstraintActive = true;
			}
		}
	}
	else
	{
		HandBounds = FBox(EForceInit::ForceInitToZero);
	}

	if (!bWasConstraintActive && bIsConstraintActive)
	{
		if (bMoveOwningActor)
		{
			// When activating snap to the goal
			GetOwner()->SetActorLocation(GoalLocation);
			GetOwner()->SetActorRotation(GoalRotation);
		}

		// Activate constraint
		OnConstraintActivated.Broadcast();
		OnBeginTracking.Broadcast(TrackedHand);
	}
	else if (bWasConstraintActive && !bIsConstraintActive)
	{
		// Deactivate constraint
		OnEndTracking.Broadcast(TrackedHand);
		OnConstraintDeactivated.Broadcast();
	}
	else if (bIsConstraintActive && OldTrackedHand != TrackedHand)
	{
		// Tracked hand changed while constraint is active
		OnEndTracking.Broadcast(OldTrackedHand);
		OnBeginTracking.Broadcast(TrackedHand);
	}
}

bool UUxtHandConstraintComponent::UpdateTrackedHand(FVector& OutPalmLocation, FQuat& OutPalmRotation)
{
	// Utility lambda for getting palm location and rotation of the TrackedHand, returns false if rejected.
	auto GetValidTransformFromTrackedHand = [this, &OutPalmLocation, &OutPalmRotation]() -> bool {
		if (IsHandUsableForConstraint(TrackedHand))
		{
			float PalmRadius;
			return UUxtHandTrackingFunctionLibrary::GetHandJointState(
				TrackedHand, EUxtHandJoint::Palm, OutPalmRotation, OutPalmLocation, PalmRadius);
		}
		return false;
	};

	// Update the tracked hand
	if (Hand == EControllerHand::Left || Hand == EControllerHand::Right)
	{
		TrackedHand = Hand;
		return GetValidTransformFromTrackedHand();
	}
	else if (Hand == EControllerHand::AnyHand)
	{
		// Try to use current tracked hand
		if (GetValidTransformFromTrackedHand())
		{
			return true;
		}

		// Tracking lost, select opposite hand
		TrackedHand = (TrackedHand == EControllerHand::Left ? EControllerHand::Right : EControllerHand::Left);
		return GetValidTransformFromTrackedHand();
	}
	else
	{
		// Unspecified hand type
		return false;
	}
}

bool UUxtHandConstraintComponent::UpdateHandBounds(const FVector& PalmLocation, const FQuat& PalmRotation)
{
	FTransform WorldFromPalm = FTransform(PalmRotation, PalmLocation);
	FTransform PalmFromWorld = WorldFromPalm.Inverse();
	HandBounds = FBox(EForceInit::ForceInitToZero);

	for (int i = 0; i < (uint8)EUxtHandJoint::Count; ++i)
	{
		EUxtHandJoint Joint = (EUxtHandJoint)i;

		FQuat JointRotation;
		FVector JointLocation;
		float JointRadius;
		if (!UUxtHandTrackingFunctionLibrary::GetHandJointState(TrackedHand, Joint, JointRotation, JointLocation, JointRadius))
		{
			continue;
		}

		// Joint position in palm coordinates
		FVector LocalLoc = PalmFromWorld.TransformPosition(JointLocation);
		// Union with box around the joint, using radius for padding
		HandBounds += FBox(LocalLoc - FVector::OneVector * JointRadius, LocalLoc + FVector::OneVector * JointRadius);
	}

	return (bool)HandBounds.IsValid;
}

bool UUxtHandConstraintComponent::UpdateGoal(const FVector& PalmLocation, const FQuat& PalmRotation)
{
	if (!HandBounds.IsValid)
	{
		return false;
	}

	FVector ZoneDirection = GetZoneDirection(PalmLocation, PalmRotation);

	// Inverse transform ray origin and direction into hand bounds space
	FVector LocalZoneDirection = PalmRotation.UnrotateVector(ZoneDirection);

	// Enlarge bounds by margin
	FBox ZoneBox = HandBounds.ExpandBy(GoalMargin);
	// Extent vector is half size, multiply by 3 to ensure the ray reaches outside the box
	float RayLength = 3.0f * ZoneBox.GetExtent().Size();
	FVector LocalHitLocation;
	// Should only fail in degenerate cases, e.g. empty bounding box.
	if (!LineBoxIntersectionInternal(ZoneBox, FVector::ZeroVector, RayLength * LocalZoneDirection, LocalHitLocation))
	{
		return false;
	}

	GoalLocation = PalmRotation.RotateVector(LocalHitLocation) + PalmLocation;

	const FTransform& CurrentTransform = GetOwner()->GetActorTransform();
	switch (RotationMode)
	{
	case EUxtHandConstraintRotationMode::None:
		GoalRotation = CurrentTransform.GetRotation();
		break;

	case EUxtHandConstraintRotationMode::LookAtCamera:
	{
		FTransform HeadPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());
		GoalRotation = FRotationMatrix::MakeFromXZ(HeadPose.GetLocation() - GoalLocation, FVector::UpVector).ToQuat();
		break;
	}

	case EUxtHandConstraintRotationMode::HandRotation:
		// Palm rotation has X facing up, rotate about Y by 90 degrees so that Z is up for consistency
		GoalRotation = PalmRotation * FRotator(-90, 0, 0).Quaternion();
		break;
	}

	return true;
}

void UUxtHandConstraintComponent::AddMovement(float DeltaTime)
{
	FVector Location = GetOwner()->GetActorLocation();
	FQuat Rotation = GetOwner()->GetActorQuat();

	FVector SmoothLoc;
	if (LocationLerpTime <= KINDA_SMALL_NUMBER)
	{
		SmoothLoc = GoalLocation;
	}
	else
	{
		float Weight = FMath::Clamp(1.0f - FMath::Exp(-DeltaTime / LocationLerpTime), 0.0f, 1.0f);
		SmoothLoc = FMath::Lerp(Location, GoalLocation, Weight);
	}

	FQuat SmoothRot;
	if (RotationLerpTime <= KINDA_SMALL_NUMBER)
	{
		SmoothRot = GoalRotation;
	}
	else
	{
		float Weight = FMath::Clamp(1.0f - FMath::Exp(-DeltaTime / RotationLerpTime), 0.0f, 1.0f);
		SmoothRot = FMath::Lerp(Rotation, GoalRotation, Weight);
	}

	GetOwner()->SetActorLocation(SmoothLoc);
	GetOwner()->SetActorRotation(SmoothRot);
}
