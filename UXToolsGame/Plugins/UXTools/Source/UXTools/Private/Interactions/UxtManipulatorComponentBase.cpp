// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Interactions/UxtManipulatorComponentBase.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Interactions/Manipulation/UxtManipulationMoveLogic.h"
#include "Interactions/Manipulation/UxtTwoHandRotateLogic.h"
#include "Interactions/Manipulation/UxtTwoHandScaleLogic.h"
#include "Engine/World.h"

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

void UUxtManipulatorComponentBase::MoveToTargets(const FTransform &SourceTransform, FTransform &TargetTransform, bool UsePointerRotation) const
{
	FVector NewObjectLocation = MoveLogic->Update(GetPointersTransformCentroid(),
		SourceTransform.Rotator().Quaternion(),
		SourceTransform.GetScale3D(),
		UsePointerRotation,
		UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation());
	TargetTransform = FTransform(SourceTransform.GetRotation(), NewObjectLocation, SourceTransform.GetScale3D());
}

void UUxtManipulatorComponentBase::RotateAroundPivot(const FTransform &SourceTransform, const FVector &Pivot, FTransform &TargetTransform) const
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

void UUxtManipulatorComponentBase::RotateAboutAxis(const FTransform &SourceTransform, const FVector &Pivot, const FVector &Axis, FTransform &TargetTransform) const
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

void UUxtManipulatorComponentBase::SmoothTransform(const FTransform& SourceTransform, float LocationSmoothing, float RotationSmoothing, float DeltaSeconds, FTransform& TargetTransform) const
{
	FVector SmoothLoc;
	FQuat SmoothRot;

	FTransform CurTransform = GetComponentTransform();

	FVector CurLoc = CurTransform.GetLocation();
	FVector SourceLoc = SourceTransform.GetLocation();
	if (LocationSmoothing <= 0.0f)
	{
		SmoothLoc = CurLoc;
	}
	else
	{
		float Weight = FMath::Clamp(FMath::Exp(-LocationSmoothing * DeltaSeconds), 0.0f, 1.0f);
		SmoothLoc = FMath::Lerp(CurLoc, SourceLoc, Weight);
	}

	FQuat CurRot = CurTransform.GetRotation();
	FQuat SourceRot = SourceTransform.GetRotation();
	if (RotationSmoothing <= 0.0f)
	{
		SmoothRot = CurRot;
	}
	else
	{
		float Weight = FMath::Clamp(FMath::Exp(-RotationSmoothing * DeltaSeconds), 0.0f, 1.0f);
		SmoothRot = FMath::Lerp(CurRot, SourceRot, Weight);
	}

	TargetTransform.SetComponents(SmoothRot, SmoothLoc, SourceTransform.GetScale3D());
}

void UUxtManipulatorComponentBase::SetInitialTransform()
{
	InitialTransform = GetComponentTransform();

	FTransform headPose = UUxtFunctionLibrary::GetHeadPose(GetWorld());
	InitialCameraSpaceTransform = InitialTransform * headPose.Inverse();
}

void UUxtManipulatorComponentBase::ApplyTargetTransform(const FTransform &TargetTransform)
{
	FTransform currentActorTransform = GetOwner()->GetActorTransform();
	FTransform offsetTransform = GetComponentTransform() * currentActorTransform.Inverse();

	GetOwner()->SetActorTransform(TargetTransform * offsetTransform);
}

void UUxtManipulatorComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoSetInitialTransform)
	{
		OnBeginGrab.AddDynamic(this, &UUxtManipulatorComponentBase::OnManipulationStarted);
		OnEndGrab.AddDynamic(this, &UUxtManipulatorComponentBase::OnManipulationEnd);
	}
}

void UUxtManipulatorComponentBase::OnManipulationStarted(UUxtGrabTargetComponent *Grabbable, FUxtGrabPointerData GrabPointer)
{
	int NumGrabPointers = GetGrabPointers().Num();
	if (NumGrabPointers != 0)
	{
		SetInitialTransform();

		MoveLogic->Setup(GetPointersTransformCentroid(),
			GetGrabPointCentroid(GetComponentTransform()),
			GetComponentTransform(),
			UUxtFunctionLibrary::GetHeadPose(GetWorld()).GetLocation());

		if (NumGrabPointers > 1)
		{
			TwoHandRotateLogic->Setup(GetGrabPointers(), GetComponentRotation().Quaternion());
			TwoHandScaleLogic->Setup(GetGrabPointers(), GetComponentScale());
		}
	}
}

void UUxtManipulatorComponentBase::OnManipulationEnd(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
{
	int NumGrabPointers = GetGrabPointers().Num();
	if (NumGrabPointers != 1)
	{
		// make sure to update the initial transform when we switch the hand mode (currently only two to one hand supported)
		SetInitialTransform();
	}
}

