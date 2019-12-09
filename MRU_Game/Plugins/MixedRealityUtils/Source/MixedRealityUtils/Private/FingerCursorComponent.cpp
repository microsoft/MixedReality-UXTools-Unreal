// Fill out your copyright notice in the Description page of Project Settings.

#include "FingerCursorComponent.h"
#include "TouchPointer.h"
#include "MixedRealityUtils.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"


UFingerCursorComponent::UFingerCursorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	RingMaterial = FMixedRealityUtilsModule::GetDefaultCursorRingMaterial();
}

void UFingerCursorComponent::OnRegister()
{
	Super::OnRegister();

	MeshComponent = NewObject<UStaticMeshComponent>(this, TEXT("CursorMesh"));

	auto Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), NULL, TEXT("/Engine/BasicShapes/Plane")));
	check(Mesh);
	MeshComponent->SetStaticMesh(Mesh);

	check(RingMaterial);
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
	MeshComponent->RegisterComponent();
}

void UFingerCursorComponent::BeginPlay()
{
	Super::BeginPlay();

	auto Owner = GetOwner();
	TouchPointer = Owner->FindComponentByClass<UTouchPointer>();

	if (TouchPointer)
	{
		// Tick after the pointer so we use its latest state
		AddTickPrerequisiteComponent(TouchPointer);
	}
	else
	{
		UE_LOG(MixedRealityUtils, Error, TEXT("Could not find a touch pointer in actor '%s'. Finger cursor won't work properly."), *Owner->GetName());
	}
}

void UFingerCursorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TouchPointer)
	{
		FVector PointOnTarget;
		if (auto Target = TouchPointer->GetHoveredTarget(PointOnTarget))
		{
			MeshComponent->SetVisibility(true);

			const auto TargetToPointer = TouchPointer->GetComponentLocation() - PointOnTarget;
			const auto DistanceToTarget = TargetToPointer.Size();

			// Must use an epsilon to avoid unreliable rotations as we get closer to the target
			const float Epsilon = 0.000001;

			if (DistanceToTarget > Epsilon)
			{
				// Orient cursor towards target
				const FMatrix Rotation = FRotationMatrix::MakeFromZ(TargetToPointer);
				SetWorldRotation(Rotation.ToQuat());
			}

			// Scale outer radius with the distance to the target
			FName OuterRadiusParameter(TEXT("OuterRadius"));
			float Alpha = TargetToPointer.Size() / MaxDistanceToTarget;
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
