// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFingerCursorComponent.h"
#include "UXTools.h"
#include "Input/UxtNearPointerComponent.h"
#include "GameFramework/Actor.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace
{
	/**
	 * The cursor interpolates between two different transforms as it approaches the target.
	 * The first transform, which has a greater influence further away from the target, is 
	 * constructed as follows:
	 * - Location: (fingertip pos) + (tip radius) * (dir from knuckle to fingertip)
	 * - Rotation: (fingertip rot)
	 * 
	 * The second transform, Which has a greater influence closer to the target, is constructed 
	 * as follows:
	 * - Location: (fingertip pos) + (tip radius) * (dir from fingertip to point on target)
	 * - Rotation: (rot corresponding to dir from fingertip to point on target)
	 */
	FTransform GetCursorTransform(EControllerHand Hand, FVector PointOnTarget, FVector Normal, float AlignWithSurfaceDistance)
	{
		bool foundValues = true;

		FQuat IndexTipOrientation;
		FVector IndexTipPosition;
		float IndexTipRadius;
		
		foundValues &= UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius);

		FQuat IndexKnuckleOrientation;
		FVector IndexKnucklePosition;
		float IndexKnuckleRadius;

		foundValues &= UUxtHandTrackingFunctionLibrary::GetHandJointState(Hand, EUxtHandJoint::IndexProximal, IndexKnuckleOrientation, IndexKnucklePosition, IndexKnuckleRadius);

		if (!foundValues)
		{
			return FTransform::Identity;
		}

		auto FingerDir = (IndexTipPosition - IndexKnucklePosition);
		FingerDir.Normalize();

		const auto DistanceToTarget = FVector::Dist(PointOnTarget, IndexTipPosition);

		FVector Location;
		FQuat Rotation;

		if (DistanceToTarget < AlignWithSurfaceDistance)
		{
			float SlerpAmount = DistanceToTarget / AlignWithSurfaceDistance;

			FQuat FullRotation = FQuat::FindBetweenNormals(FingerDir, -Normal);
			FVector Dir = FQuat::Slerp(FullRotation, FQuat::Identity, SlerpAmount) * FingerDir;

			Location = IndexTipPosition + Dir * IndexTipRadius;
			Rotation = FQuat::Slerp((-Normal).ToOrientationQuat(), IndexTipOrientation, SlerpAmount);
		}
		else
		{
			Location = IndexTipPosition + FingerDir * IndexTipRadius;
			Rotation = IndexTipOrientation;
		}

		return FTransform(Rotation, Location);
	}
}

UUxtFingerCursorComponent::UUxtFingerCursorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("StaticMesh'/UXTools/Pointers/Meshes/SM_FingerTipCursor'"));
	check(MeshFinder.Object);
	SetStaticMesh(MeshFinder.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/UXTools/Pointers/Materials/MI_FingerTipCursor"));
	check(MaterialFinder.Object);
	SetMaterial(0, MaterialFinder.Object);
	
	// Remain hidden until we see a valid poke target
	SetHiddenInGame(true);
}

void UUxtFingerCursorComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	UUxtNearPointerComponent* HandPointer = Owner->FindComponentByClass<UUxtNearPointerComponent>();
	HandPointerWeak = HandPointer;

	if (HandPointer)
	{
		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(HandPointer);
	}
	else
	{
		UE_LOG(UXTools, Error, TEXT("Could not find a near pointer in actor '%s'. Finger cursor won't work properly."), *Owner->GetName());
	}

	UMaterialInterface* Material = GetMaterial(0);
	FingerMaterialInstance = CreateDynamicMaterialInstance(0, Material);

	SetRadius(CursorScale);
}

void UUxtFingerCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtNearPointerComponent* HandPointer = HandPointerWeak.Get())
	{
		FVector PointOnTarget;
		FVector SurfaceNormal;
		FTransform PointerTransform;

		UObject* Target = HandPointer->GetFocusedPokeTarget(PointOnTarget, SurfaceNormal);
		if (Target)
		{
			PointerTransform = HandPointer->GetPokePointerTransform();
		}
		else if (bShowOnGrabTargets)
		{
			Target = HandPointer->GetFocusedGrabTarget(PointOnTarget, SurfaceNormal);

			if (Target)
			{
				PointerTransform = HandPointer->GetGrabPointerTransform();
			}
		}

		if (Target)
		{
			const float DistanceToTarget = FVector::Dist(PointOnTarget, PointerTransform.GetLocation());

			// Must use an epsilon to avoid unreliable rotations as we get closer to the target
			const float Epsilon = 0.000001;

			if (DistanceToTarget > Epsilon)
			{
				SetWorldTransform(GetCursorTransform(HandPointer->Hand, PointOnTarget, SurfaceNormal, AlignWithSurfaceDistance));												
			}

			const float DistanceOffset = 1.0f;
			// Scale radius with the distance to the target
			float Alpha = (DistanceToTarget - DistanceOffset) / MaxDistanceToTarget;
			FingerMaterialInstance->SetScalarParameterValue(FName("Proximity Distance"), Alpha);

			if (bHiddenInGame)
			{
				SetHiddenInGame(false);
			}
		}
		else if(!bHiddenInGame)
		{
			// Hide mesh when the pointer has no target
			SetHiddenInGame(true);
		}
	}
}
