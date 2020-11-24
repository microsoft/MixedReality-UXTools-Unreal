// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtGenericManipulatorComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Constraints/UxtConstraintManager.h"
#include "Engine/World.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtHandInteractionActor.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/Manipulation/UxtTwoHandRotateLogic.h"
#include "Interactions/Manipulation/UxtTwoHandScaleLogic.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Utils/UxtMathUtilsFunctionLibrary.h"

// Sets default values for this component's properties
UUxtGenericManipulatorComponent::UUxtGenericManipulatorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;

	// Default values
	OneHandRotationMode = EUxtOneHandRotationMode::MaintainOriginalRotation;
	TwoHandTransformModes = static_cast<int32>(EUxtTransformMode::Translation | EUxtTransformMode::Rotation | EUxtTransformMode::Scaling);
	ReleaseBehavior = static_cast<int32>(EUxtReleaseBehavior::KeepVelocity | EUxtReleaseBehavior::KeepAngularVelocity);
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

	// Register begin / end grab callbacks.
	OnBeginGrab.AddDynamic(this, &UUxtGenericManipulatorComponent::OnGrab);
	OnEndGrab.AddDynamic(this, &UUxtGenericManipulatorComponent::OnRelease);

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
	case EUxtOneHandRotationMode::RotateAboutGrabPoint:
	{
		const FTransform GripTransform = UUxtGrabPointerDataFunctionLibrary::GetGripTransform(PrimaryPointerData);
		OutTargetTransform.SetRotation((PrimaryPointerData.GripToObject * GripTransform).GetRotation());
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

		OutTargetTransform.SetRotation(OrientationTwist);
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
	if (!(GrabModes & static_cast<int32>(EUxtGrabMode::OneHanded)))
	{
		return;
	}

	FTransform TargetTransform = GetTargetComponent()->GetComponentTransform();
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
	if (!(GrabModes & static_cast<int32>(EUxtGrabMode::TwoHanded)))
	{
		return;
	}

	FTransform TargetTransform = GetTargetComponent()->GetComponentTransform();

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

void UUxtGenericManipulatorComponent::OnGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	if (GetGrabPointers().Num() == 1)
	{
		if (UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(GetTargetComponent()))
		{
			if (Target->IsSimulatingPhysics())
			{
				Target->SetSimulatePhysics(false);
				bWasSimulatingPhysics = true;
			}
			else
			{
				bWasSimulatingPhysics = false;
			}
		}
	}
}

void UUxtGenericManipulatorComponent::OnRelease(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	if (bWasSimulatingPhysics && GetGrabPointers().Num() == 0)
	{
		if (UPrimitiveComponent* Target = Cast<UPrimitiveComponent>(GetTargetComponent()))
		{
			Target->SetSimulatePhysics(true);

			const bool bKeepLinearVelocity = ReleaseBehavior & static_cast<int32>(EUxtReleaseBehavior::KeepVelocity);
			const bool bKeepAngularVelocity = ReleaseBehavior & static_cast<int32>(EUxtReleaseBehavior::KeepAngularVelocity);

			if (const AUxtHandInteractionActor* Hand = Cast<AUxtHandInteractionActor>(
					GrabPointer.NearPointer ? GrabPointer.NearPointer->GetOwner() : GrabPointer.FarPointer->GetOwner()))
			{
				Target->SetPhysicsLinearVelocity(bKeepLinearVelocity ? Hand->GetHandVelocity() : FVector::ZeroVector);
				Target->SetPhysicsAngularVelocityInDegrees(bKeepAngularVelocity ? Hand->GetHandAngularVelocity() : FVector::ZeroVector);
			}
		}
	}
}
