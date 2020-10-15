// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Behaviors/UxtFollowComponent.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Utils/UxtInternalFunctionLibrary.h"

namespace
{
	float SimplifyAngle(float Angle)
	{
		while (Angle > PI)
		{
			Angle -= 2 * PI;
		}

		while (Angle < -PI)
		{
			Angle += 2 * PI;
		}

		return Angle;
	}

	float AngleBetweenOnPlane(FVector From, FVector To, FVector Normal)
	{
		From.Normalize();
		To.Normalize();
		Normal.Normalize();

		FVector Right = FVector::CrossProduct(Normal, From);
		FVector Forward = FVector::CrossProduct(Right, Normal);

		float Angle = FMath::Atan2(FVector::DotProduct(To, Right), FVector::DotProduct(To, Forward));

		return SimplifyAngle(Angle);
	}

	float AngleBetweenVectorAndPlane(FVector Vec, FVector Normal)
	{
		Vec.Normalize();
		Normal.Normalize();
		return (PI / 2) - FMath::Acos(FVector::DotProduct(Vec, Normal));
	}

	bool AngularClamp(
		FTransform FollowTransform, bool bIgnoreVertical, float MaxHorizontalDegrees, float MaxVerticalDegrees, FVector& CurrentToTarget)
	{
		if (CurrentToTarget.Size() <= 0)
		{
			// No need to clamp
			return false;
		}

		// This is the meat of the leashing algorithm. The goal is to ensure that the reference's forward
		// vector remains within the bounds set by the leashing parameters. To do this, determine the angles
		// between toTarget and the leashing bounds about the global Z axis and the reference's X axis.
		// If toTarget falls within the leashing bounds, then we don't have to modify it.
		// Otherwise, we apply a correction rotation to bring it within bounds.

		FVector FollowForward = FollowTransform.GetUnitAxis(EAxis::X);
		FVector FollowRight = FollowTransform.GetUnitAxis(EAxis::Y);

		bool bAngularClamped = false;

		// X-axis leashing
		// Leashing around the reference's X axis only makes sense if the reference isn't gravity aligned.
		if (bIgnoreVertical)
		{
			float Angle = AngleBetweenOnPlane(CurrentToTarget, FollowForward, FollowRight);
			CurrentToTarget = FQuat(FollowRight, Angle) * CurrentToTarget;
		}
		else
		{
			// These are negated because Unreal is left-handed
			float Angle = -AngleBetweenOnPlane(CurrentToTarget, FollowForward, FollowRight);
			float MinMaxAngle = FMath::DegreesToRadians(MaxVerticalDegrees) * 0.5f;

			if (Angle < -MinMaxAngle)
			{
				CurrentToTarget = FQuat(FollowRight, -MinMaxAngle - Angle) * CurrentToTarget;
				bAngularClamped = true;
			}
			else if (Angle > MinMaxAngle)
			{
				CurrentToTarget = FQuat(FollowRight, MinMaxAngle - Angle) * CurrentToTarget;
				bAngularClamped = true;
			}
		}

		// Z-axis leashing
		{
			float Angle = AngleBetweenVectorAndPlane(CurrentToTarget, FollowRight);
			float MinMaxAngle = FMath::DegreesToRadians(MaxHorizontalDegrees) * 0.5f;

			if (Angle < -MinMaxAngle)
			{
				CurrentToTarget = FQuat(FVector::UpVector, -MinMaxAngle - Angle) * CurrentToTarget;
				bAngularClamped = true;
			}
			else if (Angle > MinMaxAngle)
			{
				CurrentToTarget = FQuat(FVector::UpVector, MinMaxAngle - Angle) * CurrentToTarget;
				bAngularClamped = true;
			}
		}

		return bAngularClamped;
	}

	bool DistanceClamp(
		FTransform FollowTransform, bool bMoveToDefault, bool bIgnorePitch, float MinDistance, float DefaultDistance, float MaxDistance,
		FVector& CurrentToTarget)
	{
		FVector GoalDirection = CurrentToTarget;
		GoalDirection.Normalize();

		float CurrentDistance = CurrentToTarget.Size();

		if (bIgnorePitch)
		{
			// If we don't account for pitch offset, the casted object will float up/down as the reference
			// gets closer to it because we will still be casting in the direction of the pitched offset.
			// To fix this, only modify the XZ position of the object.

			MinDistance = GoalDirection.Size2D() * MinDistance;
			MaxDistance = GoalDirection.Size2D() * MaxDistance;

			float CurrentDistance2D = CurrentToTarget.Size2D();

			// scale goal direction so scalar multiplication works with 2D distances
			GoalDirection *= (CurrentDistance / CurrentDistance2D);

			CurrentDistance = CurrentDistance2D;
		}

		float ClampedDistance = CurrentDistance;

		if (bMoveToDefault)
		{
			if (CurrentDistance < MinDistance || CurrentDistance > MaxDistance)
			{
				ClampedDistance = DefaultDistance;
			}
		}
		else
		{
			ClampedDistance = FMath::Clamp(CurrentDistance, MinDistance, MaxDistance);
		}

		CurrentToTarget = GoalDirection * ClampedDistance;

		return CurrentDistance != ClampedDistance;
	}

	void ApplyVerticalClamp(float MaxVerticalDistance, FVector& CurrentToTarget)
	{
		if (MaxVerticalDistance != 0)
		{
			CurrentToTarget.Z = FMath::Clamp(CurrentToTarget.Z, -MaxVerticalDistance, MaxVerticalDistance);
		}
	}

	void ComputeOrientation(
		EUxtFollowOrientBehavior OrientationType, FVector FollowPosition, FVector CurrentToTarget, FQuat& CurrentRotation)
	{
		switch (OrientationType)
		{
		case EUxtFollowOrientBehavior::FaceCamera:
			CurrentRotation = (-CurrentToTarget).ToOrientationQuat();
			break;
		default:
			break;
		}
	}

	void ApplyPitchOffset(const float PitchOffset, FTransform& FollowTransform)
	{
		FVector Forward = FollowTransform.GetUnitAxis(EAxis::X);
		Forward.Z = 0;
		FVector Left = FollowTransform.GetUnitAxis(EAxis::Y);
		Forward = FQuat(Left, FMath::DegreesToRadians(PitchOffset)) * Forward;
		FollowTransform.SetRotation(Forward.ToOrientationQuat());
	}

	FVector SmoothTo(FVector Source, FVector Goal, float DeltaTime, float LerpTime)
	{
		return UUxtInternalFunctionLibrary::Slerp(Source, Goal, LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime);
	}

	FQuat SmoothTo(FQuat Source, FQuat Goal, float DeltaTime, float LerpTime)
	{
		return FQuat::Slerp(Source, Goal, LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime);
	}

	bool PassedOrientationDeadzone(FVector CurrentToTarget, FQuat CurrentRotation, FVector FollowPosition, float DeadzoneDegrees)
	{
		FVector LeashForward = CurrentRotation * FVector::ForwardVector;

		FVector LeashToFollow = CurrentToTarget;
		LeashToFollow.Normalize();

		float Angle = FMath::Abs(AngleBetweenOnPlane(LeashForward, LeashToFollow, FVector::UpVector));

		return FMath::RadiansToDegrees(Angle) > DeadzoneDegrees;
	}
} // namespace

UUxtFollowComponent::UUxtFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}

void UUxtFollowComponent::Recenter()
{
	bRecenterNextUpdate = true;
}

void UUxtFollowComponent::BeginPlay()
{
	Super::BeginPlay();

	Recenter();

	WorkingTransform = GetOwner()->GetTransform();

	if (bAutoActivate)
	{
		UpdateLeashing();
		UpdateTransformToGoal(true);
	}
}

void UUxtFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateLeashing();
	UpdateTransformToGoal(!bInterpolatePose, DeltaTime);
}

FTransform UUxtFollowComponent::GetFollowTransform()
{
	if (ActorToFollow)
	{
		return ActorToFollow->GetTransform();
	}

	return UUxtFunctionLibrary::GetHeadPose(GetWorld());
}

void UUxtFollowComponent::UpdateLeashing()
{
	FTransform FollowTransform = GetFollowTransform();

	FVector FollowPosition = FollowTransform.GetLocation();

	if (bIgnoreCameraPitchAndRoll && !bUseFixedVerticalOffset)
	{
		ApplyPitchOffset(PitchOffset, FollowTransform);
	}

	// Update starting goal leash values
	ToTarget = WorkingTransform.GetLocation() - FollowTransform.GetLocation();
	TargetRotation = WorkingTransform.GetRotation();

	// Determine the current position of the element
	bool bAngularClamped = false;
	if (bRecenterNextUpdate)
	{
		ToTarget = FollowTransform.GetUnitAxis(EAxis::X) * DefaultDistance;
		bRecenterNextUpdate = false;
	}
	// Angularly clamp to determine goal direction to place the element
	else
	{
		if (bIgnoreAngleClamp)
		{
			float CurrentDistance = ToTarget.Size();
			ToTarget = FollowTransform.GetUnitAxis(EAxis::X) * CurrentDistance;
		}
		else
		{
			bAngularClamped =
				AngularClamp(FollowTransform, bIgnoreCameraPitchAndRoll, MaxViewHorizontalDegrees, MaxViewVerticalDegrees, ToTarget);
		}
	}

	// Distance clamp to determine goal position to place the element
	bool bDistanceClamped = false;
	if (!bIgnoreDistanceClamp)
	{
		bDistanceClamped = DistanceClamp(
			FollowTransform, bAngularClamped, bIgnoreCameraPitchAndRoll, MinimumDistance, DefaultDistance, MaximumDistance, ToTarget);
		ApplyVerticalClamp(VerticalMaxDistance, ToTarget);
	}

	if (bUseFixedVerticalOffset)
	{
		ToTarget.Z = (FollowPosition.Z + FixedVerticalOffset) - WorkingTransform.GetLocation().Z;
	}

	// Figure out goal rotation of the element based on orientation setting
	FQuat NewGoalRotation = FQuat::Identity;
	EUxtFollowOrientBehavior OrientationBehavior = OrientationType;
	if (bAngularClamped || bDistanceClamped || OrientationType == EUxtFollowOrientBehavior::FaceCamera ||
		PassedOrientationDeadzone(ToTarget, TargetRotation, FollowTransform.GetLocation(), OrientToCameraDeadzoneDegrees))
	{
		OrientationBehavior = EUxtFollowOrientBehavior::FaceCamera;
	}

	ComputeOrientation(OrientationBehavior, FollowTransform.GetLocation(), ToTarget, TargetRotation);
}

void UUxtFollowComponent::UpdateTransformToGoal(bool bSkipInterpolation, float DeltaTime)
{
	FVector FollowPosition = GetFollowTransform().GetLocation();

	if (bSkipInterpolation)
	{
		WorkingTransform.SetLocation(FollowPosition + ToTarget);
		WorkingTransform.SetRotation(TargetRotation);
	}
	else
	{
		FVector CurrentPosition = GetOwner()->GetTransform().GetLocation();
		FVector CurrentDirection = CurrentPosition - FollowPosition;
		FQuat CurrentRotation = GetOwner()->GetTransform().GetRotation();
		WorkingTransform.SetLocation(FollowPosition + SmoothTo(CurrentDirection, ToTarget, DeltaTime, LerpTime));
		WorkingTransform.SetRotation(SmoothTo(CurrentRotation, TargetRotation, DeltaTime, LerpTime));
	}

	GetOwner()->SetActorTransform(WorkingTransform, false);
}
