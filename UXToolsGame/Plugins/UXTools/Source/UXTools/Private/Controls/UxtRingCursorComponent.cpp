// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtRingCursorComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UUxtRingCursorComponent::UUxtRingCursorComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);

	// Used to update material parameters in response to scale changes
	bWantsOnUpdateTransform = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/UXTools/Pointers/Meshes/SM_Cursor_Focus_geo"));
	check(MeshFinder.Object);
	FocusMesh = MeshFinder.Object;
	SetStaticMesh(MeshFinder.Object);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PressMeshFinder(TEXT("/UXTools/Pointers/Meshes/SM_Cursor_Press_geo"));
	check(PressMeshFinder.Object);
	PressMesh = PressMeshFinder.Object;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RingMaterialFinder(TEXT("/UXTools/Pointers/Materials/M_Light"));
	check(RingMaterialFinder.Object);
	SetMaterial(0, RingMaterialFinder.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BorderMaterialFinder(TEXT("/UXTools/Pointers/Materials/M_Shadow"));
	check(BorderMaterialFinder.Object);
	SetMaterial(1, BorderMaterialFinder.Object);
}

void UUxtRingCursorComponent::OnRegister()
{
	Super::OnRegister();

	MaterialInstanceRing = CreateDynamicMaterialInstance(0, GetMaterial(0));
	MaterialInstanceBorder = CreateDynamicMaterialInstance(1, GetMaterial(1));

	// Update material parameters
	SetRingColor(RingColor);
	SetBorderColor(BorderColor);

	// Intialize radius from current scale
	OnUpdateTransform(EUpdateTransformFlags::None);
}

void UUxtRingCursorComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	// Ignore transform update if it originates from SetRadius()
	if (!bSettingRadius)
	{
		FVector Scale = GetComponentScale().GetAbs();
		float NewRadius = 0.5f * FMath::Min(Scale.Y, Scale.Z);
		if (NewRadius != Radius)
		{
			SetRadius(NewRadius, false);
		}
	}
}

void UUxtRingCursorComponent::SetRingColor(FColor NewRingColor)
{
	static FName RingColorParameter = "RingColor";
	MaterialInstanceRing->SetVectorParameterValue(RingColorParameter, NewRingColor);
	RingColor = NewRingColor;
}

void UUxtRingCursorComponent::SetBorderColor(FColor NewBorderColor)
{
	static FName BorderColorParameter = "BorderColor";
	MaterialInstanceBorder->SetVectorParameterValue(BorderColorParameter, NewBorderColor);
	BorderColor = NewBorderColor;
}

void UUxtRingCursorComponent::SetRadius(float NewRadius)
{
	SetRadius(NewRadius, true);
}

void UUxtRingCursorComponent::SetRadius(float NewRadius, bool bUpdateScale)
{
	Radius = NewRadius;

	if (bUpdateScale)
	{
		bSettingRadius = true;
		SetWorldScale3D(FVector(2.0f * Radius));
		bSettingRadius = false;
	}
}
