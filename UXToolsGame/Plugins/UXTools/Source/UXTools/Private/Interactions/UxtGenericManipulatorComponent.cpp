// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtGenericManipulatorComponent.h"

#include "Components/PrimitiveComponent.h"
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
	OneHandRotationMode = EUxtOneHandRotationMode::RotateAboutGrabPoint;
	TwoHandTransformModes = static_cast<int32>(EUxtTransformMode::Translation | EUxtTransformMode::Rotation | EUxtTransformMode::Scaling);
	ReleaseBehavior = static_cast<int32>(EUxtReleaseBehavior::KeepVelocity | EUxtReleaseBehavior::KeepAngularVelocity);
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

	const FTransform GripTransform = UUxtGrabPointerDataFunctionLibrary::GetGripTransform(PrimaryPointerData);
	OutTargetTransform.SetRotation((PrimaryPointerData.GripToObject * GripTransform).GetRotation());

	return true;
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
	ApplyConstraints(TargetTransform, EUxtTransformMode::Scaling, true, IsNearManipulation());

	GetOneHandRotation(TargetTransform, TargetTransform);
	ApplyConstraints(TargetTransform, EUxtTransformMode::Rotation, true, IsNearManipulation());

	MoveToTargets(TargetTransform, TargetTransform, OneHandRotationMode != EUxtOneHandRotationMode::RotateAboutObjectCenter);
	ApplyConstraints(TargetTransform, EUxtTransformMode::Translation, true, IsNearManipulation());

	SmoothTransform(TargetTransform, LerpTime, LerpTime, DeltaTime, TargetTransform);

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
		ApplyConstraints(TargetTransform, EUxtTransformMode::Scaling, false, IsNearManipulation());
	}

	if (!!(TwoHandTransformModes & static_cast<int32>(EUxtTransformMode::Rotation)))
	{
		GetTwoHandRotation(TargetTransform, TargetTransform);
		ApplyConstraints(TargetTransform, EUxtTransformMode::Rotation, false, IsNearManipulation());
	}

	if (!!(TwoHandTransformModes & static_cast<int32>(EUxtTransformMode::Translation)))
	{
		MoveToTargets(TargetTransform, TargetTransform, true);
		ApplyConstraints(TargetTransform, EUxtTransformMode::Translation, false, IsNearManipulation());
	}

	SmoothTransform(TargetTransform, LerpTime, LerpTime, DeltaTime, TargetTransform);

	ApplyTargetTransform(TargetTransform);
}

void UUxtGenericManipulatorComponent::OnGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	InitializeConstraints(TransformTarget);

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
