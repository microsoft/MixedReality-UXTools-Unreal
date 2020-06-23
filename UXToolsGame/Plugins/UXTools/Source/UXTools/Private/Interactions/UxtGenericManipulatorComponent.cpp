// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Interactions/Manipulation/UxtTwoHandRotateLogic.h"
#include "Interactions/Manipulation/UxtTwoHandScaleLogic.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Constraints/UxtConstraintManager.h"

#include "Engine/World.h"

// Sets default values for this component's properties
UUxtGenericManipulatorComponent::UUxtGenericManipulatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = ETickingGroup::TG_PostPhysics;

	// Default values
	ManipulationModes = static_cast<int32>(EUxtGenericManipulationMode::OneHanded | EUxtGenericManipulationMode::TwoHanded);
	OneHandRotationMode = EUxtOneHandRotationMode::MaintainOriginalRotation;
	TwoHandTransformModes = static_cast<int32>(EUxtTransformMode::Translation | EUxtTransformMode::Rotation | EUxtTransformMode::Scaling);
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

void UUxtGenericManipulatorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Set the user defined transform target if specified
	if (USceneComponent* Reference = UUxtFunctionLibrary::GetSceneComponentFromReference(TargetComponent, GetOwner()))
	{
		TransformTarget = Reference;
	}
	
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

	OutTargetTransform = InSourceTransform;
	switch (OneHandRotationMode)
	{
		case EUxtOneHandRotationMode::MaintainOriginalRotation:
		{
			return true;
		}

		case EUxtOneHandRotationMode::RotateAboutObjectCenter:
		{
			FVector objectCenterAsPivot = InSourceTransform.GetLocation();
			FRotator DeltaRot = UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(InSourceTransform, PrimaryPointerData);
			OutTargetTransform.SetRotation(UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(InSourceTransform, DeltaRot, objectCenterAsPivot).GetRotation());
			return true;
		}

		case EUxtOneHandRotationMode::RotateAboutGrabPoint:
		{
			FVector GrabPointAsPivot = UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(InSourceTransform, PrimaryPointerData);
			FRotator DeltaRot = UUxtGrabPointerDataFunctionLibrary::GetRotationOffset(InSourceTransform, PrimaryPointerData);
			FQuat Orientation = UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(InSourceTransform, DeltaRot, GrabPointAsPivot).GetRotation();
			OutTargetTransform.SetRotation(Orientation);
			return true;
		}

		case EUxtOneHandRotationMode::MaintainRotationToUser:
		{
			FQuat Orientation = GetViewInvariantRotation();
			OutTargetTransform.SetRotation(Orientation);
			return true;
		}

		case EUxtOneHandRotationMode::GravityAlignedMaintainRotationToUser:
		{
			FQuat Orientation = GetViewInvariantRotation();

			// Decompose and keep only the gravity-aligned twist component of the orientation
			FQuat OrientationSwing, OrientationTwist;
			Orientation.ToSwingTwist(FVector::UpVector, OrientationSwing, OrientationTwist);

			OutTargetTransform.SetRotation(Orientation);
			return true;
		}

		case EUxtOneHandRotationMode::FaceUser:
		{
			FVector HeadLoc = UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation();
			FVector ObjectLoc = InSourceTransform.GetLocation();

			// Make the object face the user
			FVector Forward = HeadLoc - ObjectLoc;
			FQuat Orientation = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();

			OutTargetTransform.SetRotation(Orientation);
			return true;
		}

		case EUxtOneHandRotationMode::FaceAwayFromUser:
		{
			FVector HeadLoc = UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation();
			FVector ObjectLoc = InSourceTransform.GetLocation();

			// Make the object face away from the user
			FVector Forward = ObjectLoc - HeadLoc;
			FQuat Orientation = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();

			OutTargetTransform.SetRotation(Orientation); 
			return true;
		}
	}

	return false;
}

bool UUxtGenericManipulatorComponent::GetTwoHandRotation(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const
{
	OutTargetTransform = InSourceTransform;
	OutTargetTransform.SetRotation(TwoHandRotateLogic->Update(GetGrabPointers()));
	return true;
}

bool UUxtGenericManipulatorComponent::GetTwoHandScale(const FTransform& InSourceTransform, FTransform& OutTargetTransform) const
{
	OutTargetTransform = InSourceTransform;
	OutTargetTransform.SetScale3D(TwoHandScaleLogic->Update(GetGrabPointers()));
	return true;
}

bool UUxtGenericManipulatorComponent::IsNearManipulation() const
{
	return GetGrabPointers()[0].NearPointer != nullptr;
}

void UUxtGenericManipulatorComponent::UpdateOneHandManipulation(float DeltaTime)
{
	if (!(ManipulationModes & static_cast<int32>(EUxtGenericManipulationMode::OneHanded)))
	{
		return;
	}

	FTransform TargetTransform = InitialTransform;
	Constraints->ApplyScaleConstraints(TargetTransform, true, IsNearManipulation());

	GetOneHandRotation(TargetTransform, TargetTransform);
	Constraints->ApplyRotationConstraints(TargetTransform, true, IsNearManipulation());
	
	MoveToTargets(TargetTransform, TargetTransform, OneHandRotationMode != EUxtOneHandRotationMode::RotateAboutObjectCenter);
	Constraints->ApplyTranslationConstraints(TargetTransform, true, IsNearManipulation());

	SmoothTransform(TargetTransform, Smoothing, Smoothing, DeltaTime, TargetTransform);

	ApplyTargetTransform(TargetTransform);
}

void UUxtGenericManipulatorComponent::UpdateTwoHandManipulation(float DeltaTime)
{
	if (!(ManipulationModes & static_cast<int32>(EUxtGenericManipulationMode::TwoHanded)))
	{
		return;
	}

	FTransform TargetTransform = InitialTransform;

	if (!!(TwoHandTransformModes & static_cast<int32>(EUxtTransformMode::Scaling)))
	{
		GetTwoHandScale(TargetTransform, TargetTransform);
		Constraints->ApplyScaleConstraints(TargetTransform, false, IsNearManipulation());
	}

	if (!!(TwoHandTransformModes & static_cast<int32>(EUxtTransformMode::Rotation)))
	{
		GetTwoHandRotation(TargetTransform, TargetTransform);
		Constraints->ApplyRotationConstraints(TargetTransform, false, IsNearManipulation());
	}

	if (!!(TwoHandTransformModes & static_cast<int32>(EUxtTransformMode::Translation)))
	{
		MoveToTargets(TargetTransform, TargetTransform, true);
		Constraints->ApplyTranslationConstraints(TargetTransform, false, IsNearManipulation());
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
