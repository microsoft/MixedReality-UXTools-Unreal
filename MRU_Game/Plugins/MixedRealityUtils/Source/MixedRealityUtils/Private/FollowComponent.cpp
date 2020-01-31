// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "FollowComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

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

	float AngleBetweenOnYXPlane(FVector From, FVector To)
	{
		float Angle = FMath::Atan2(To.X, To.Y) - FMath::Atan2(From.X, From.Y);
		return SimplifyAngle(Angle);
	}

	float AngleBetweenOnYZPlane(FVector From, FVector To)
	{
		float Angle = FMath::Atan2(To.Z, To.Y) - FMath::Atan2(From.Z, From.Y);
		return SimplifyAngle(Angle);
	}

	float AngleBetweenOnAxis(FVector From, FVector To, FVector Axis)
	{
		FQuat AxisQuat = Axis.ToOrientationQuat().Inverse();
		FVector Vector1 = AxisQuat * From;
		FVector Vector2 = AxisQuat * To;
		return AngleBetweenOnYZPlane(Vector1, Vector2);
	}

	bool AngularClamp(
		FVector RefPosition,
		FQuat RefRotation,
		FVector CurrentPosition,
		bool bIgnoreVertical,
		float MaxHorizontalDegrees,
		float MaxVerticalDegrees,
		FVector& RefForward)
	{
		FVector ToTarget = CurrentPosition - RefPosition;
		float CurrentDistance = ToTarget.Size();
		if (CurrentDistance <= 0)
		{
			// No need to clamp
			return false;
		}

		ToTarget.Normalize();

		// Start off with a rotation towards the target. If it's within leashing bounds, we can leave it alone.
		FQuat Rotation = ToTarget.ToOrientationQuat();

		// This is the meat of the leashing algorithm. The goal is to ensure that the reference's forward
		// vector remains within the bounds set by the leashing parameters. To do this, determine the angles
		// between toTarget and the leashing bounds about the global Z axis and the reference's X axis.
		// If toTarget falls within the leashing bounds, then we don't have to modify it.
		// Otherwise, we apply a correction rotation to bring it within bounds.

		FVector CurrentRefForward = RefRotation * FVector::ForwardVector;
		FVector RefRight = RefRotation * FVector::RightVector;

		bool bAngularClamped = false;

		// X-axis leashing
		// Leashing around the reference's X axis only makes sense if the reference isn't gravity aligned.
		if (bIgnoreVertical)
		{
			float Angle = AngleBetweenOnAxis(ToTarget, CurrentRefForward, RefRight);
			Rotation = FQuat(RefRight, Angle) * Rotation;
		}
		else
		{
			FVector Min = FQuat(RefRight, FMath::DegreesToRadians(MaxVerticalDegrees  * 0.5f)) * RefForward;
			FVector Max = FQuat(RefRight, FMath::DegreesToRadians(-MaxVerticalDegrees * 0.5f)) * RefForward;

			float MinAngle = AngleBetweenOnAxis(ToTarget, Min, RefRight);
			float MaxAngle = AngleBetweenOnAxis(ToTarget, Max, RefRight);

			if (MinAngle < 0)
			{
				Rotation = FQuat(RefRight, MinAngle) * Rotation;
				bAngularClamped = true;
			}
			else if (MaxAngle > 0)
			{
				Rotation = FQuat(RefRight, MaxAngle) * Rotation;
				bAngularClamped = true;
			}
		}

		// Z-axis leashing
		{
			FVector Min = FQuat(FVector::UpVector, FMath::DegreesToRadians(-MaxHorizontalDegrees * 0.5f)) * RefForward;
			FVector Max = FQuat(FVector::UpVector, FMath::DegreesToRadians(MaxHorizontalDegrees * 0.5f)) * RefForward;

			// These are negated because Unity is left-handed
			float MinAngle = -AngleBetweenOnYXPlane(ToTarget, Min);
			float MaxAngle = -AngleBetweenOnYXPlane(ToTarget, Max);

			if (MinAngle > 0)
			{
				Rotation = FQuat(FVector::UpVector, MinAngle) * Rotation;
				bAngularClamped = true;
			}
			else if (MaxAngle < 0)
			{
				Rotation = FQuat(FVector::UpVector, MaxAngle) * Rotation;
				bAngularClamped = true;
			}
		}

		RefForward = Rotation * FVector::ForwardVector;

		return bAngularClamped;
	}

	bool DistanceClamp(
		float DeltaTime,
		float MinDistance,
		float DefaultDistanceIn,
		float MaxDistance,
		bool bMaintainPitch,
		FVector CurrentPosition,
		FVector RefPosition,
		FVector RefForward,
		bool bInterpolateToDefaultDistance,
		float MoveToDefaultDistanceLerpTime,
		FVector& ClampedPosition)
	{
		float ClampedDistance;
		float CurrentDistance = FVector::Distance(CurrentPosition, RefPosition);
		FVector Direction = RefForward;

		if (bMaintainPitch)
		{
			// If we don't account for pitch offset, the casted object will float up/down as the reference
			// gets closer to it because we will still be casting in the direction of the pitched offset.
			// To fix this, only modify the XZ position of the object.

			FVector DirectionYX = RefForward;
			DirectionYX.Z = 0;
			DirectionYX.Normalize();

			FVector RefToElementYX = CurrentPosition - RefPosition;
			RefToElementYX.Z = 0;
			float DesiredDistanceYX = RefToElementYX.Size();

			FVector MinDistanceYXVector = RefForward * MinDistance;
			MinDistanceYXVector.Z = 0;
			float MinDistanceYX = MinDistanceYXVector.Size();

			FVector MaxDistanceYXVector = RefForward * MaxDistance;
			MaxDistanceYXVector.Z = 0;
			float MaxDistanceYX = MaxDistanceYXVector.Size();

			DesiredDistanceYX = FMath::Clamp(DesiredDistanceYX, MinDistanceYX, MaxDistanceYX);

			if (bInterpolateToDefaultDistance)
			{
				FVector DefaultDistanceYXVector = Direction * DefaultDistanceIn;
				DefaultDistanceYXVector.Z = 0;
				float DefaulltDistanceYX = DefaultDistanceYXVector.Size();

				float interpolationRate = FMath::Min(MoveToDefaultDistanceLerpTime * 60.0f * DeltaTime, 1000.0f);
				DesiredDistanceYX = DesiredDistanceYX + (interpolationRate * (DefaulltDistanceYX - DesiredDistanceYX));
			}

			FVector DesiredPosition = RefPosition + DirectionYX * DesiredDistanceYX;
			float DesiredHeight = RefPosition.Z + RefForward.Z * MaxDistance;
			DesiredPosition.Z = DesiredHeight;

			Direction = DesiredPosition - RefPosition;
			ClampedDistance = Direction.Size();
			Direction /= ClampedDistance;

			ClampedDistance = FMath::Max(MinDistance, ClampedDistance);
		}
		else
		{
			ClampedDistance = CurrentDistance;

			if (bInterpolateToDefaultDistance)
			{
				float InterpolationRate = FMath::Min(MoveToDefaultDistanceLerpTime * 60.0f * DeltaTime, 1000.0f);
				ClampedDistance = ClampedDistance + (InterpolationRate * (DefaultDistanceIn - ClampedDistance));
			}

			ClampedDistance = FMath::Clamp(ClampedDistance, MinDistance, MaxDistance);
		}

		ClampedPosition = RefPosition + Direction * ClampedDistance;

		return !ClampedPosition.Equals(CurrentPosition, 1.0f);
	}

	void ComputeOrientation(
		EFollowOrientBehavior DefaultOrientationType,
		FVector FollowPosition,
		FVector GoalLocation,
		FQuat PreviousGoalRotation,
		FQuat& Orientation)
	{
		switch (DefaultOrientationType)
		{
		case EFollowOrientBehavior::FaceCamera:
			Orientation = (GoalLocation - FollowPosition).ToOrientationQuat();
			break;
		case EFollowOrientBehavior::WorldLock:
			Orientation = PreviousGoalRotation;
			break;
		default:
			break;
		}
	}

	void GetReferenceInfo(
		FVector PreviousRefPosition,
		FVector CurrentRefPosition,
		FQuat CurrentRefRotation,
		float MaxVerticalDistance,
		bool bIgnoreCameraPitchAndRoll,
		float PitchOffset,
		bool ApplyVerticalClamp,
		FVector& RefPosition,
		FQuat& RefRotation,
		FVector& RefForward)
	{
		RefPosition = CurrentRefPosition;
		RefRotation = CurrentRefRotation;
		if (bIgnoreCameraPitchAndRoll)
		{
			FVector Forward = CurrentRefRotation * FVector::ForwardVector;
			Forward.Z = 0;
			RefRotation = Forward.ToOrientationQuat();
			if (PitchOffset != 0)
			{
				FVector Left = RefRotation * FVector::LeftVector;
				Forward = FQuat(Left, FMath::DegreesToRadians(PitchOffset)) * Forward;
				RefRotation = Forward.ToOrientationQuat();
			}
		}

		RefForward = RefRotation * FVector::ForwardVector;

		// Apply vertical clamp on reference
		if (ApplyVerticalClamp && MaxVerticalDistance > 0)
		{
			RefPosition.Z = FMath::Clamp(PreviousRefPosition.Z, CurrentRefPosition.Z - MaxVerticalDistance, CurrentRefPosition.Z + MaxVerticalDistance);
		}
	}

	FVector SmoothTo(FVector Source, FVector Goal, float DeltaTime, float LerpTime)
	{
		return FMath::Lerp(Source, Goal, LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime);
	}

	FQuat SmoothTo(FQuat Source, FQuat Goal, float DeltaTime, float LerpTime)
	{
		return FMath::Lerp(Source, Goal, LerpTime == 0.0f ? 1.0f : DeltaTime / LerpTime);
	}

	EFollowOrientBehavior GetOrientationType(
		FVector GoalLocation,
		bool bAngularClamp,
		bool bDistanceClamp,
		FVector CurrentForward,
		FVector FollowPosition,
		float OrientToCameraDeadzoneDegrees,
		EFollowOrientBehavior DefaultOrientationType)
	{
		if (bAngularClamp || bDistanceClamp || DefaultOrientationType == EFollowOrientBehavior::FaceCamera)
		{
			return EFollowOrientBehavior::FaceCamera;
		}
		else
		{
			FVector CamForward = CurrentForward;

			FVector NodeToCamera = GoalLocation - FollowPosition;
			NodeToCamera.Normalize();

			float Angle = FMath::Abs(AngleBetweenOnYXPlane(CamForward, NodeToCamera));

			if (FMath::RadiansToDegrees(Angle) > OrientToCameraDeadzoneDegrees)
			{
				return EFollowOrientBehavior::FaceCamera;
			}
		}

		return DefaultOrientationType;
	}
}

UFollowComponent::UFollowComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFollowComponent::Recenter()
{
	bRecenterNextUpdate = true;
}

void UFollowComponent::BeginPlay()
{
	Super::BeginPlay();

	Recenter();

	WorkingPosition = GetOwner()->GetTransform().GetLocation();
	WorkingRotation = GetOwner()->GetTransform().GetRotation();
	PreviousReferencePosition = FVector::ZeroVector;

	PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (PlayerCameraManager == nullptr)
	{
		UE_LOG(LogClass, Error, TEXT("No PlayerCameraManager"));
		return;
	}

	bSkipInterpolation = true;
}

void UFollowComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PlayerCameraManager == nullptr)
	{
		return;
	}

	FVector FollowPosition = PlayerCameraManager->GetCameraLocation();
	FQuat FollowRotation = FQuat(PlayerCameraManager->GetCameraRotation());

	if (!bHaveValidCamera)
	{
		bHaveValidCamera = true;

		if (FollowPosition.IsZero())
		{
			// On the first frame, the camera position is 0, 0, 0, so skip that frame.
			return;
		}
	}

	FVector CurrentReferencePosition = FVector::ZeroVector;
	FQuat CurrentReferenceRotation = FQuat::Identity;
	FVector ReferenceForward = FVector::ZeroVector;
	GetReferenceInfo(
		PreviousReferencePosition,
		FollowPosition,
		FollowRotation,
		VerticalMaxDistance,
		bIgnoreCameraPitchAndRoll,
		PitchOffset,
		!bRecenterNextUpdate && !bSkipInterpolation,
		CurrentReferencePosition,
		CurrentReferenceRotation,
		ReferenceForward);

	// Determine the current position of the element
	FVector CurrentPosition = WorkingPosition;
	FVector GoalDirection = CurrentReferenceRotation * FVector::ForwardVector;
	bool bAngularClamped = false;
	if (bRecenterNextUpdate)
	{
		CurrentPosition = CurrentReferencePosition + ReferenceForward * DefaultDistance;
	}
	// Angularly clamp to determine goal direction to place the element
	else if (!bIgnoreAngleClamp)
	{
		bAngularClamped = AngularClamp(
			CurrentReferencePosition,
			CurrentReferenceRotation,
			CurrentPosition,
			bIgnoreCameraPitchAndRoll,
			MaxViewHorizontalDegrees,
			MaxViewVerticalDegrees,
			GoalDirection);
	}

	// Distance clamp to determine goal position to place the element
	FVector NewGoalPosition = CurrentPosition;
	bool bDistanceClamped = false;
	if (!bIgnoreDistanceClamp)
	{
		bDistanceClamped = DistanceClamp(
			DeltaTime,
			MinimumDistance,
			DefaultDistance,
			MaximumDistance,
			(PitchOffset != 0),
			CurrentPosition,
			CurrentReferencePosition,
			GoalDirection,
			bAngularClamped,
			MoveToDefaultDistanceLerpTime,
			NewGoalPosition);
	}
	else
	{
		NewGoalPosition = CurrentReferencePosition + GoalDirection * FVector::Distance(CurrentPosition, CurrentReferencePosition);
	}

	// Figure out goal rotation of the element based on orientation setting
	FQuat NewGoalRotation = FQuat::Identity;
	EFollowOrientBehavior OrientationBehavior = GetOrientationType(
		NewGoalPosition,
		bAngularClamped,
		bDistanceClamped,
		WorkingRotation.GetForwardVector(),
		ReferenceForward,
		OrientToCameraDeadzoneDegrees,
		OrientationType);

	ComputeOrientation(
		OrientationBehavior,
		FollowPosition,
		NewGoalPosition,
		PreviousRotation,
		NewGoalRotation);

	PreviousRotation = GoalRotation;
	GoalPosition = NewGoalPosition;
	GoalRotation = NewGoalRotation;

	PreviousReferencePosition = CurrentReferencePosition;
	PreviousReferenceRotation = CurrentReferenceRotation;
	bRecenterNextUpdate = false;

	UpdateTransformToGoal(DeltaTime);
	bSkipInterpolation = false;
}

void UFollowComponent::UpdateTransformToGoal(float DeltaTime)
{
	if (bSkipInterpolation)
	{
		WorkingPosition = GoalPosition;
		WorkingRotation = GoalRotation;
	}
	else
	{
		FVector CurrentPosition = GetOwner()->GetTransform().GetLocation();
		FQuat CurrentRotation = GetOwner()->GetTransform().GetRotation();
		WorkingPosition = SmoothTo(CurrentPosition, GoalPosition, DeltaTime, .5f);
		WorkingRotation = SmoothTo(CurrentRotation, GoalRotation, DeltaTime, .5f);
	}

	GetOwner()->SetActorLocationAndRotation(WorkingPosition, WorkingRotation, false);
}