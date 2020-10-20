// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtFingerCursorComponent.h"

#include "UXTools.h"

#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Input/UxtNearPointerComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const float InitalCursorFadeScaler = 2;
	const float TargetCursorFadeScaler = 1;
	const float CursorFadeSpeed = 2;

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

		foundValues &= UUxtHandTrackingFunctionLibrary::GetHandJointState(
			Hand, EUxtHandJoint::IndexTip, IndexTipOrientation, IndexTipPosition, IndexTipRadius);

		FQuat IndexKnuckleOrientation;
		FVector IndexKnucklePosition;
		float IndexKnuckleRadius;

		foundValues &= UUxtHandTrackingFunctionLibrary::GetHandJointState(
			Hand, EUxtHandJoint::IndexProximal, IndexKnuckleOrientation, IndexKnucklePosition, IndexKnuckleRadius);

		if (!foundValues)
		{
			return FTransform::Identity;
		}

		FVector FingerDir = (IndexTipPosition - IndexKnucklePosition);
		FingerDir.Normalize();

		const float DistanceToTarget = FVector::Dist(PointOnTarget, IndexTipPosition);

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
} // namespace

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

	const AActor* const Owner = GetOwner();
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

	// Initialize the fade to 200% to that it can be interpolated to 100% when enabled. Note, the cursor begins to appear at around 130%.
	CursorFadeScaler = InitalCursorFadeScaler;
}

void UUxtFingerCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtNearPointerComponent* HandPointer = HandPointerWeak.Get())
	{
		if (HandPointer->IsActive())
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

			SetWorldTransform(
				GetCursorTransform(HandPointer->Hand, PointOnTarget, SurfaceNormal, Target ? AlignWithSurfaceDistance : -1.0f));

			float Alpha = 1.0f;

			// Scale the radius/translucency with the distance to the target.
			if (Target)
			{
				const float DistanceToTarget = FVector::Dist(PointOnTarget, PointerTransform.GetLocation());
				Alpha = DistanceToTarget / HandPointer->ProximityRadius;
			}

			FingerMaterialInstance->SetScalarParameterValue(FName("Proximity Distance"), Alpha * CursorFadeScaler);
			CursorFadeScaler = FMath::Clamp(CursorFadeScaler - DeltaTime * CursorFadeSpeed, TargetCursorFadeScaler, InitalCursorFadeScaler);

			// Ensure the cursor is not hidden when the hand pointer is active.
			if (bHiddenInGame)
			{
				SetHiddenInGame(false);
			}
		}
		else if (!bHiddenInGame)
		{
			// Hide mesh when the pointer is inactive.
			SetHiddenInGame(true);

			CursorFadeScaler = InitalCursorFadeScaler;
		}
	}
}
