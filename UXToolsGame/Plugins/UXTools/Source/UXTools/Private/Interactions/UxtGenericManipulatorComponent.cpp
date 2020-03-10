// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"
#include "Utils/UxtFunctionLibrary.h"

#include "Engine/World.h"

// Sets default values for this component's properties
UUxtGenericManipulatorComponent::UUxtGenericManipulatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	// Default values
	ManipulationModes = (1 << (uint8)EUxtGenericManipulationMode::OneHanded) | (1 << (uint8)EUxtGenericManipulationMode::TwoHanded);
	OneHandRotationMode = EUxtOneHandRotationMode::MaintainOriginalRotation;
	TwoHandTransformModes = (1 << (uint8)EUxtTwoHandTransformMode::Translation) | (1 << (uint8)EUxtTwoHandTransformMode::Rotation) | (1 << (uint8)EUxtTwoHandTransformMode::Scaling);
	Smoothing = 100.0f;
}

void UUxtGenericManipulatorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	int NumPointers = GetGrabPointers().Num();
	if (NumPointers == 0)
	{
		return;
	}
	else if (NumPointers == 1)
	{
		UpdateOneHandManipulation(DeltaTime);
	}
	else if (NumPointers == 2)
	{
		UpdateTwoHandManipulation(DeltaTime);
	}
	else
	{
		// 3+ hands not supported
	}
}

FQuat UUxtGenericManipulatorComponent::GetViewInvariantRotation() const
{
	FRotator CameraSpaceYawPitchRotation = UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetRotation().Rotator();
	// Ignore roll
	CameraSpaceYawPitchRotation.Roll = 0.0f;

	return CameraSpaceYawPitchRotation.Quaternion() * InitialCameraSpaceTransform.GetRotation();
}

bool UUxtGenericManipulatorComponent::GetOneHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const
{
	bool bHasPrimaryPointer;
	FUxtGrabPointerData PrimaryPointerData;
	GetPrimaryGrabPointer(bHasPrimaryPointer, PrimaryPointerData);

	if (!bHasPrimaryPointer)
	{
		return false;
	}

	switch (OneHandRotationMode)
	{
		case EUxtOneHandRotationMode::MaintainOriginalRotation:
		{
			OutTargetTransform = InSourceTransform;
			return true;
		}

		case EUxtOneHandRotationMode::RotateAboutObjectCenter:
		{
			FRotator DeltaRot = UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(InSourceTransform, PrimaryPointerData);
			FVector Pivot = InSourceTransform.GetLocation();
			OutTargetTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(InSourceTransform, DeltaRot, Pivot);
			return true;
		}

		case EUxtOneHandRotationMode::RotateAboutGrabPoint:
		{
			FRotator DeltaRot = UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(InSourceTransform, PrimaryPointerData);
			FVector Pivot = UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(InSourceTransform, PrimaryPointerData);
			OutTargetTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(InSourceTransform, DeltaRot, Pivot);
			return true;
		}

		case EUxtOneHandRotationMode::MaintainRotationToUser:
		{
			FQuat Orientation = GetViewInvariantRotation();
			OutTargetTransform = FTransform(Orientation, InSourceTransform.GetLocation(), InSourceTransform.GetScale3D());
			return true;
		}

		case EUxtOneHandRotationMode::GravityAlignedMaintainRotationToUser:
		{
			FQuat Orientation = GetViewInvariantRotation();

			// Decompose and keep only the gravity-aligned twist component of the orientation
			FQuat OrientationSwing, OrientationTwist;
			Orientation.ToSwingTwist(FVector::UpVector, OrientationSwing, OrientationTwist);

			OutTargetTransform = FTransform(OrientationTwist, InSourceTransform.GetLocation(), InSourceTransform.GetScale3D());
			return true;
		}

		case EUxtOneHandRotationMode::FaceUser:
		{
			FVector HeadLoc = UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation();
			FVector ObjectLoc = InSourceTransform.GetLocation();

			// Make the object face the user
			FVector Forward = HeadLoc - ObjectLoc;
			FQuat Orientation = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();

			OutTargetTransform = FTransform(Orientation, InSourceTransform.GetLocation(), InSourceTransform.GetScale3D());
			return true;
		}

		case EUxtOneHandRotationMode::FaceAwayFromUser:
		{
			FVector HeadLoc = UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation();
			FVector ObjectLoc = InSourceTransform.GetLocation();

			// Make the object face away from the user
			FVector Forward = ObjectLoc - HeadLoc;
			FQuat Orientation = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();

			OutTargetTransform = FTransform(Orientation, InSourceTransform.GetLocation(), InSourceTransform.GetScale3D());
			return true;
		}
	}

	return false;
}

bool UUxtGenericManipulatorComponent::GetTwoHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const
{
	bool bHasPrimaryPointer, bHasSecondaryPointer;
	FUxtGrabPointerData PrimaryPointerData, SecondaryPointerData;
	GetPrimaryGrabPointer(bHasPrimaryPointer, PrimaryPointerData);
	GetSecondaryGrabPointer(bHasSecondaryPointer, SecondaryPointerData);

	if (!bHasPrimaryPointer || !bHasSecondaryPointer)
	{
		return false;
	}

	FVector PrimarySourceLoc = UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(InSourceTransform, PrimaryPointerData);
	FVector SecondarySourceLoc = UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(InSourceTransform, SecondaryPointerData);
	FVector PrimaryTargetLoc = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(PrimaryPointerData);
	FVector SecondaryTargetLoc = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(SecondaryPointerData);

	// Compute minimal rotation to align the line between hands from the source to the target points.
	FVector SourceDelta = SecondarySourceLoc - PrimarySourceLoc;
	FVector TargetDelta = SecondaryTargetLoc - PrimaryTargetLoc;
	FRotator DeltaRot = FQuat::FindBetween(SourceDelta, TargetDelta).Rotator();

	// Rotate transform with target centroid as the pivot point.
	FVector Centroid = 0.5 * (PrimaryTargetLoc + SecondaryTargetLoc);
	OutTargetTransform = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(InSourceTransform, DeltaRot, Centroid);
	return true;
}

void UUxtGenericManipulatorComponent::UpdateOneHandManipulation(float DeltaTime)
{
	if (!(ManipulationModes & (1 << (uint8)EUxtGenericManipulationMode::OneHanded)))
	{
		return;
	}

	FTransform TargetTransform = InitialTransform;

	MoveToTargets(TargetTransform, TargetTransform);

	GetOneHandRotation(TargetTransform, TargetTransform);

	SmoothTransform(TargetTransform, Smoothing, Smoothing, DeltaTime, TargetTransform);

	ApplyTargetTransform(TargetTransform);
}

void UUxtGenericManipulatorComponent::UpdateTwoHandManipulation(float DeltaTime)
{
	if (!(ManipulationModes & (1 << (uint8)EUxtGenericManipulationMode::TwoHanded)))
	{
		return;
	}

	FTransform TargetTransform = InitialTransform;

	if (!!(TwoHandTransformModes & (1 << (uint8)EUxtTwoHandTransformMode::Translation)))
	{
		MoveToTargets(TargetTransform, TargetTransform);
	}

	if (!!(TwoHandTransformModes & (1 << (uint8)EUxtTwoHandTransformMode::Rotation)))
	{
		GetTwoHandRotation(TargetTransform, TargetTransform);
	}

	SmoothTransform(TargetTransform, Smoothing, Smoothing, DeltaTime, TargetTransform);

	ApplyTargetTransform(TargetTransform);
}

float UUxtGenericManipulatorComponent::GetSmoothing() const
{
	return Smoothing;
}

void UUxtGenericManipulatorComponent::SetSmoothing(float NewSmoothing)
{
	Smoothing = FMath::Max(NewSmoothing, 0.0f);
}
