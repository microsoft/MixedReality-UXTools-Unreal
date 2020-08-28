// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtManipulatorComponentBase.h"

#include "Constraints/UxtConstraintManager.h"
#include "Engine/World.h"
#include "Interactions/Manipulation/UxtManipulationMoveLogic.h"
#include "Interactions/Manipulation/UxtTwoHandRotateLogic.h"
#include "Interactions/Manipulation/UxtTwoHandScaleLogic.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Utils/UxtInternalFunctionLibrary.h"

UUxtManipulatorComponentBase::UUxtManipulatorComponentBase()
{
	MoveLogic = new UxtManipulationMoveLogic();
	TwoHandRotateLogic = new UxtTwoHandManipulationRotateLogic();
	TwoHandScaleLogic = new UxtTwoHandManipulationScaleLogic();
}

UUxtManipulatorComponentBase::~UUxtManipulatorComponentBase()
{
	delete TwoHandScaleLogic;
	delete TwoHandRotateLogic;
	delete MoveLogic;
}

void UUxtManipulatorComponentBase::MoveToTargets(
	const FTransform& SourceTransform, FTransform& TargetTransform, bool UsePointerRotation) const
{
	FVector NewObjectLocation = MoveLogic->Update(
		GetPointerCentroid(), SourceTransform.Rotator().Quaternion(), SourceTransform.GetScale3D(), UsePointerRotation,
		UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation());
	TargetTransform = FTransform(SourceTransform.GetRotation(), NewObjectLocation, SourceTransform.GetScale3D());
}

void UUxtManipulatorComponentBase::RotateAroundPivot(
	const FTransform& SourceTransform, const FVector& Pivot, FTransform& TargetTransform) const
{
	TargetTransform = SourceTransform;

	if (GetGrabPointers().Num() == 0)
	{
		return;
	}

	if (GetGrabPointers().Num() > 1)
	{
		// TODO this will require solving an Eigenvalue problem (SVD) to find the least-squares solution
		return;
	}

	FVector grab = UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(SourceTransform, GetGrabPointers()[0]);
	FVector target = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GetGrabPointers()[0]);
	// Make relative to pivot
	grab -= Pivot;
	target -= Pivot;

	// Use minimal-angle rotation from grab vector to target vector
	FQuat minRot = FQuat::FindBetween(grab, target);

	TargetTransform = SourceTransform;
	TargetTransform *= FTransform(-Pivot);
	TargetTransform *= FTransform(minRot.Rotator());
	TargetTransform *= FTransform(Pivot);
}

void UUxtManipulatorComponentBase::RotateAboutAxis(
	const FTransform& SourceTransform, const FVector& Pivot, const FVector& Axis, FTransform& TargetTransform) const
{
	TargetTransform = SourceTransform;

	if (GetGrabPointers().Num() == 0)
	{
		return;
	}

	if (GetGrabPointers().Num() > 1)
	{
		// TODO this will require solving an Eigenvalue problem (SVD) to find the least-squares solution
		return;
	}

	FVector grab = UUxtGrabPointerDataFunctionLibrary::GetGrabLocation(SourceTransform, GetGrabPointers()[0]);
	FVector target = UUxtGrabPointerDataFunctionLibrary::GetTargetLocation(GetGrabPointers()[0]);
	// Make relative to pivot
	grab -= Pivot;
	target -= Pivot;

	// Compute the rotation around the axis by using a twist-swing decomposition of the minimal rotation
	FQuat minRot = FQuat::FindBetween(grab, target);
	FQuat twist, swing;
	minRot.ToSwingTwist(Axis, swing, twist);

	TargetTransform = SourceTransform;
	TargetTransform *= FTransform(-Pivot);
	TargetTransform *= FTransform(twist.Rotator());
	TargetTransform *= FTransform(Pivot);
}

FTransform UUxtManipulatorComponentBase::SmoothTransform(
	const FTransform& TargetTransform, float LocationSmoothingFactor, float RotationSmoothingFactor, float DeltaSeconds) const
{
	FTransform SmoothedTransform;

	const FTransform& CurTransform = TransformTarget->GetComponentTransform();

	const FVector CurLoc = CurTransform.GetLocation();
	const FVector SourceLoc = TargetTransform.GetLocation();
	SmoothedTransform.SetLocation(UUxtInternalFunctionLibrary::SmoothLerp(CurLoc, SourceLoc, LocationSmoothingFactor, DeltaSeconds));

	const FQuat CurRot = CurTransform.GetRotation();
	const FQuat SourceRot = TargetTransform.GetRotation();
	SmoothedTransform.SetRotation(UUxtInternalFunctionLibrary::SmoothLerp(CurRot, SourceRot, RotationSmoothingFactor, DeltaSeconds));

	SmoothedTransform.SetScale3D(TargetTransform.GetScale3D());

	return SmoothedTransform;
}

void UUxtManipulatorComponentBase::SetInitialTransform()
{
	InitialTransform = TransformTarget->GetComponentTransform();

	FTransform headPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());
	InitialCameraSpaceTransform = InitialTransform * headPose.Inverse();

	Constraints->Initialize(InitialTransform);
}

void UUxtManipulatorComponentBase::ApplyTargetTransform(const FTransform& TargetTransform)
{
	TransformTarget->SetWorldTransform(TargetTransform);
	OnUpdateTransform.Broadcast(TransformTarget, TargetTransform);
}

USceneComponent* UUxtManipulatorComponentBase::GetTargetComponent()
{
	return TransformTarget;
}

void UUxtManipulatorComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (!TransformTarget)
	{
		TransformTarget = GetOwner()->GetRootComponent();
	}

	if (bAutoSetInitialTransform)
	{
		OnBeginGrab.AddDynamic(this, &UUxtManipulatorComponentBase::OnManipulationStarted);
		OnEndGrab.AddDynamic(this, &UUxtManipulatorComponentBase::OnManipulationEnd);
	}

	Constraints = new UxtConstraintManager(*GetOwner());
}

void UUxtManipulatorComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	delete Constraints;
	Super::EndPlay(EndPlayReason);
}

void UUxtManipulatorComponentBase::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Constraints->Update(TransformTarget->GetComponentTransform());
}

void UUxtManipulatorComponentBase::OnManipulationStarted(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const int NumGrabPointers = GetGrabPointers().Num();

	if (NumGrabPointers != 0)
	{
		UpdateManipulationLogic(NumGrabPointers);
	}
}

void UUxtManipulatorComponentBase::OnManipulationEnd(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	const int NumGrabPointers = GetGrabPointers().Num();

	if (NumGrabPointers == 1)
	{
		// Update the manipulation logic when we switch the hand mode (currently only two to one hand supported)
		UpdateManipulationLogic(NumGrabPointers);
	}
}

void UUxtManipulatorComponentBase::UpdateManipulationLogic(int NumGrabPointers)
{
	SetInitialTransform();

	MoveLogic->Setup(
		GetPointerCentroid(), GetGrabPointCentroid(GetComponentTransform()).GetLocation(), TransformTarget->GetComponentTransform(),
		UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation());

	if (NumGrabPointers > 1)
	{
		TwoHandRotateLogic->Setup(GetGrabPointers(), TransformTarget->GetComponentRotation().Quaternion());
		TwoHandScaleLogic->Setup(GetGrabPointers(), TransformTarget->GetComponentScale());
	}
}
