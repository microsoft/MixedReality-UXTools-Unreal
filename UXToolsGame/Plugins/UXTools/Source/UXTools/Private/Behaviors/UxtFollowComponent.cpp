// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Behaviors/UxtFollowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Utils/UxtFunctionLibrary.h"

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

	bool AngularClamp(FTransform FollowTransform, bool bIgnoreVertical, float MaxHorizontalDegrees, float MaxVerticalDegrees, FTransform& CurrentTransform)
	{
		FVector ToTarget = CurrentTransform.GetLocation() - FollowTransform.GetLocation();
		if (ToTarget.Size() <= 0)
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
			float Angle = AngleBetweenOnPlane(ToTarget, FollowForward, FollowRight);
			ToTarget = FQuat(FollowRight, Angle) * ToTarget;
		}
		else
		{
			// These are negated because Unreal is left-handed
			float Angle = -AngleBetweenOnPlane(ToTarget, FollowForward, FollowRight);
			float MinMaxAngle = FMath::DegreesToRadians(MaxVerticalDegrees) * 0.5f;

			if (Angle < -MinMaxAngle)
			{
				ToTarget = FQuat(FollowRight, -MinMaxAngle - Angle) * ToTarget;
				bAngularClamped = true;
			}
			else if (Angle > MinMaxAngle)
			{
				ToTarget = FQuat(FollowRight, MinMaxAngle - Angle) * ToTarget;
				bAngularClamped = true;
			}
		}

		// Z-axis leashing
		{
			float Angle = AngleBetweenVectorAndPlane(ToTarget, FollowRight);
			float MinMaxAngle = FMath::DegreesToRadians(MaxHorizontalDegrees) * 0.5f;

			if (Angle < -MinMaxAngle)
			{
				ToTarget = FQuat(FVector::UpVector, -MinMaxAngle - Angle) * ToTarget;
				bAngularClamped = true;
			}
			else if (Angle > MinMaxAngle)
			{
				ToTarget = FQuat(FVector::UpVector, MinMaxAngle - Angle) * ToTarget;
				bAngularClamped = true;
			}
		}

		CurrentTransform.SetLocation(FollowTransform.GetLocation() + ToTarget);

		return bAngularClamped;
	}

	bool DistanceClamp(FTransform FollowTransform, bool bMoveToDefault, bool bIgnorePitch, float MinDistance, float DefaultDistance, float MaxDistance, FTransform& CurrentTransform)
 	{
		FVector GoalDirection = CurrentTransform.GetLocation() - FollowTransform.GetLocation();
		GoalDirection.Normalize();

		float CurrentDistance = FVector::Distance(CurrentTransform.GetLocation(), FollowTransform.GetLocation());

		if (bIgnorePitch)
		{
			// If we don't account for pitch offset, the casted object will float up/down as the reference
			// gets closer to it because we will still be casting in the direction of the pitched offset.
			// To fix this, only modify the XZ position of the object.

			MinDistance = GoalDirection.Size2D() * MinDistance;
			MaxDistance = GoalDirection.Size2D() * MaxDistance;

			float CurrentDistance2D = FVector::DistXY(CurrentTransform.GetLocation(), FollowTransform.GetLocation());

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

		CurrentTransform.SetLocation(FollowTransform.GetLocation() + GoalDirection * ClampedDistance);

		return CurrentDistance != ClampedDistance;
	}

	void ApplyVerticalClamp(FVector FollowPosition,float MaxVerticalDistance, FTransform& CurrentTransform)
	{
		if (MaxVerticalDistance != 0)
		{
			FVector CurrentPosition = CurrentTransform.GetLocation();
			CurrentPosition.Z = FMath::Clamp(CurrentPosition.Z, FollowPosition.Z - MaxVerticalDistance, FollowPosition.Z + MaxVerticalDistance);
			CurrentTransform.SetLocation(CurrentPosition);
		}
	}

	void ComputeOrientation(EUxtFollowOrientBehavior OrientationType, FVector FollowPosition, FTransform& CurrentTransform)
	{
		switch (OrientationType)
		{
		case EUxtFollowOrientBehavior::FaceCamera:
			CurrentTransform.SetRotation((FollowPosition - CurrentTransform.GetLocation()).ToOrientationQuat());
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
		return FMath::Lerp(Source, Goal, LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime);
	}

	FQuat SmoothTo(FQuat Source, FQuat Goal, float DeltaTime, float LerpTime)
	{
		return FMath::Lerp(Source, Goal, LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime);
	}

	bool PassedOrientationDeadzone(FTransform CurrentTransform, FVector FollowPosition, float DeadzoneDegrees)
	{
		FVector CamForward = CurrentTransform.GetUnitAxis(EAxis::X);

		FVector NodeToCamera = CurrentTransform.GetLocation() - FollowPosition;
		NodeToCamera.Normalize();

		float Angle = FMath::Abs(AngleBetweenOnPlane(CamForward, NodeToCamera, FVector::UpVector));

		return FMath::RadiansToDegrees(Angle) > DeadzoneDegrees;
	}
}

UUxtFollowComponent::UUxtFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

	UpdateLeashing();
	UpdateTransformToGoal(true);
}

void UUxtFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateLeashing();
	UpdateTransformToGoal(!bInterpolatePose, DeltaTime);
}


void UUxtFollowComponent::UpdateLeashing()
{
	FTransform FollowTransform;

	if (ActorToFollow)
	{
		FollowTransform = ActorToFollow->GetTransform();
	}
	else
	{
		FollowTransform = UUxtFunctionLibrary::GetHeadPose(GetWorld());
	}

	FVector FollowPosition = FollowTransform.GetLocation();

	if (bIgnoreCameraPitchAndRoll)
	{
		ApplyPitchOffset(PitchOffset, FollowTransform);
	}

	// Determine the current position of the element
	GoalTransform = WorkingTransform;
	bool bAngularClamped = false;
	if (bRecenterNextUpdate)
	{
		GoalTransform.SetLocation(FollowTransform.GetLocation() + FollowTransform.GetUnitAxis(EAxis::X) * DefaultDistance);
		bRecenterNextUpdate = false;
	}
	// Angularly clamp to determine goal direction to place the element
	else
	{
		if (bIgnoreAngleClamp)
		{
			float CurrentDistance = FVector::Dist(FollowTransform.GetLocation(), GoalTransform.GetLocation());
			GoalTransform.SetLocation(FollowTransform.GetLocation() + FollowTransform.GetUnitAxis(EAxis::X) * CurrentDistance);
		}
		else
		{
			bAngularClamped = AngularClamp(FollowTransform, bIgnoreCameraPitchAndRoll, MaxViewHorizontalDegrees, MaxViewVerticalDegrees, GoalTransform);
		}
	}

	// Distance clamp to determine goal position to place the element
	bool bDistanceClamped = false;
	if (!bIgnoreDistanceClamp)
	{
		bDistanceClamped = DistanceClamp(FollowTransform, bAngularClamped, bIgnoreCameraPitchAndRoll, MinimumDistance, DefaultDistance, MaximumDistance, GoalTransform);
		ApplyVerticalClamp(FollowPosition, VerticalMaxDistance, GoalTransform);
	}

	// Figure out goal rotation of the element based on orientation setting
	FQuat NewGoalRotation = FQuat::Identity;
	EUxtFollowOrientBehavior OrientationBehavior = OrientationType;
	if (bAngularClamped || bDistanceClamped ||
		OrientationType == EUxtFollowOrientBehavior::FaceCamera ||
		PassedOrientationDeadzone(GoalTransform, FollowTransform.GetLocation(), OrientToCameraDeadzoneDegrees))
	{
		OrientationBehavior = EUxtFollowOrientBehavior::FaceCamera;
	}

	ComputeOrientation(OrientationBehavior, FollowTransform.GetLocation(), GoalTransform);
}

void UUxtFollowComponent::UpdateTransformToGoal(bool bSkipInterpolation, float DeltaTime)
{
	if (bSkipInterpolation)
	{
		WorkingTransform = GoalTransform;
	}
	else
	{
		FVector CurrentPosition = GetOwner()->GetTransform().GetLocation();
		FQuat CurrentRotation = GetOwner()->GetTransform().GetRotation();
		WorkingTransform.SetLocation(SmoothTo(CurrentPosition, GoalTransform.GetLocation(), DeltaTime, .5f));
		WorkingTransform.SetRotation(SmoothTo(CurrentRotation, GoalTransform.GetRotation(), DeltaTime, .5f));
	}

	GetOwner()->SetActorTransform(WorkingTransform, false);
}