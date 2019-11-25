// Fill out your copyright notice in the Description page of Project Settings.

#include "ManipulatorComponentBase.h"
#include "MixedRealityToolsFunctionLibrary.h"
#include "GrabbableComponent.h"
#include "TouchPointer.h"

void UManipulatorComponentBase::MoveToTargets(const FTransform &SourceTransform, FTransform &TargetTransform) const
{
	FVector centerGrab = GetGrabPointCentroid(SourceTransform);
	FVector centerTarget = GetTargetCentroid();
	TargetTransform = SourceTransform * FTransform(centerTarget - centerGrab);
}

void UManipulatorComponentBase::RotateAroundPivot(const FTransform &SourceTransform, const FVector &Pivot, FTransform &TargetTransform) const
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

	if (!ensure(GetGrabPointers()[0].Pointer != nullptr))
	{
		return;
	}

	FVector grab = UGrabPointerDataFunctionLibrary::GetGrabLocation(SourceTransform, GetGrabPointers()[0]);
	FVector target = UGrabPointerDataFunctionLibrary::GetTargetLocation(GetGrabPointers()[0]);
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

void UManipulatorComponentBase::RotateAboutAxis(const FTransform &SourceTransform, const FVector &Pivot, const FVector &Axis, FTransform &TargetTransform) const
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

	if (!ensure(GetGrabPointers()[0].Pointer != nullptr))
	{
		return;
	}

	FVector grab = UGrabPointerDataFunctionLibrary::GetGrabLocation(SourceTransform, GetGrabPointers()[0]);
	FVector target = UGrabPointerDataFunctionLibrary::GetTargetLocation(GetGrabPointers()[0]);
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

void UManipulatorComponentBase::SetInitialTransform()
{
	InitialTransform = GetComponentTransform();

	FTransform headPose = UMixedRealityToolsFunctionLibrary::GetHeadPose(GetWorld());
	InitialCameraSpaceTransform = InitialTransform * headPose.Inverse();
}

void UManipulatorComponentBase::ApplyTargetTransform(const FTransform &TargetTransform)
{
	FTransform currentActorTransform = GetOwner()->GetActorTransform();
	FTransform offsetTransform = GetComponentTransform() * currentActorTransform.Inverse();

	GetOwner()->SetActorTransform(TargetTransform * offsetTransform);
}

void UManipulatorComponentBase::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoSetInitialTransform)
	{
		OnBeginGrab.AddDynamic(this, &UManipulatorComponentBase::InitTransformOnFirstPointer);
	}
}

bool UManipulatorComponentBase::GetClosestPointOnSurface_Implementation(const FVector & Point, FVector & OutPointOnSurface)
{
	OutPointOnSurface = Point;
	float ClosestPointDistanceSqr = -1.f;

	TInlineComponentArray<UPrimitiveComponent*> Components;
	GetOwner()->GetComponents(Components);

	// This is exactly what happens inside AActor::ActorGetDistanceToCollision except we're skipping the collision response check
	// because the pointer will usually be configured to overlap with this actor's primitives, not block.
	for (int32 ComponentIndex = 0; ComponentIndex < Components.Num(); ComponentIndex++)
	{
		UPrimitiveComponent* Primitive = Components[ComponentIndex];
		if (Primitive->IsRegistered() && Primitive->IsCollisionEnabled())
		{
			FVector ClosestPoint;
			float DistanceSqr = -1.f;

			if (!Primitive->GetSquaredDistanceToCollision(Point, DistanceSqr, ClosestPoint))
			{
				// Invalid result, impossible to be better than ClosestPointDistance
				continue;
			}

			if ((ClosestPointDistanceSqr < 0.f) || (DistanceSqr < ClosestPointDistanceSqr))
			{
				ClosestPointDistanceSqr = DistanceSqr;
				OutPointOnSurface = ClosestPoint;

				// If we're inside collision, we're not going to find anything better, so abort search we've got our best find.
				if (DistanceSqr <= KINDA_SMALL_NUMBER)
				{
					break;
				}
			}
		}
	}

	return ClosestPointDistanceSqr >= 0.f;
}

void UManipulatorComponentBase::InitTransformOnFirstPointer(UGrabbableComponent *Grabbable, FGrabPointerData GrabPointer)
{
	if (GetGrabPointers().Num() == 1)
	{
		SetInitialTransform();
	}
}
