// Fill out your copyright notice in the Description page of Project Settings.

#include "Controls/UxtFingerCursorComponent.h"
#include "UXTools.h"
#include "Input/UxtNearPointerComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "HandTracking/UxtHandTrackingFunctionLibrary.h"

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
	FTransform GetCursorTransform(EControllerHand Hand, FVector PointOnTarget, float AlignWithSurfaceDistance)
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

		auto ToTargetDir = PointOnTarget - IndexTipPosition;
		const auto DistanceToTarget = ToTargetDir.Size();
		ToTargetDir.Normalize();

		FVector Location;
		FQuat Rotation;

		if (DistanceToTarget < AlignWithSurfaceDistance)
		{
			float SlerpAmount = DistanceToTarget / AlignWithSurfaceDistance;

			FQuat FullRotation = FQuat::FindBetweenNormals(FingerDir, ToTargetDir);
			FVector Dir = FQuat::Slerp(FullRotation, FQuat::Identity, SlerpAmount) * FingerDir;

			Location = IndexTipPosition + Dir * IndexTipRadius;
			Rotation = FQuat::Slerp(ToTargetDir.ToOrientationQuat(), IndexTipOrientation, SlerpAmount);
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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ObjectFinder(TEXT("/UXTools/FingerPointer/FingerCursorMaterial"));
	RingMaterial = ObjectFinder.Object;
	check(RingMaterial);
}

void UUxtFingerCursorComponent::OnRegister()
{
	Super::OnRegister();

	MeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("CursorMesh"));

	auto Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Plane")));
	check(Mesh);
	MeshComponent->SetStaticMesh(Mesh);

	MaterialInstance = MeshComponent->CreateDynamicMaterialInstance(0, RingMaterial);

	// Scale plane to fit exactly the maximum outer diameter to avoid unnecessary pixel overdraw.
	{
		const float DefaultPlaneSide = 100.0f;
		const float MaxOuterDiameter = 2 * MaxOuterRadius;
		FVector PlaneScale(MaxOuterDiameter / DefaultPlaneSide);
		MeshComponent->SetWorldScale3D(PlaneScale);
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetupAttachment(this);
	MeshComponent->SetRelativeRotation(FQuat(FVector::RightVector, PI / 2));
	MeshComponent->RegisterComponent();
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
		UE_LOG(UXTools, Error, TEXT("Could not find a touch pointer in actor '%s'. Finger cursor won't work properly."), *Owner->GetName());
	}
}

void UUxtFingerCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UUxtNearPointerComponent* HandPointer = HandPointerWeak.Get())
	{
		FVector PointOnTarget;
		if (auto Target = HandPointer->GetFocusedTouchTarget(PointOnTarget))
		{
			MeshComponent->SetVisibility(true);

			FTransform TouchPointerTransform = HandPointer->GetTouchPointerTransform();
			const auto DistanceToTarget = FVector::Dist(PointOnTarget, TouchPointerTransform.GetLocation());

			// Must use an epsilon to avoid unreliable rotations as we get closer to the target
			const float Epsilon = 0.000001;

			if (DistanceToTarget > Epsilon)
			{
				SetWorldTransform(GetCursorTransform(HandPointer->GetHand(), PointOnTarget, AlignWithSurfaceDistance));
			}

			// Scale outer radius with the distance to the target
			FName OuterRadiusParameter(TEXT("OuterRadius"));
			float Alpha = DistanceToTarget / MaxDistanceToTarget;
			float OuterRadius = FMath::Lerp(MinOuterRadius, MaxOuterRadius, Alpha);
			MaterialInstance->SetScalarParameterValue(OuterRadiusParameter, OuterRadius);
		}
		else
		{
			// Hide mesh when the pointer has no target
			MeshComponent->SetVisibility(false);
		}
	}
}
