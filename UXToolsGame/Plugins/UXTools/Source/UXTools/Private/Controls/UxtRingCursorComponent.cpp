// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtRingCursorComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"


UUxtRingCursorComponent::UUxtRingCursorComponent()
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCastShadow(false);

	// Used to update material parameters in response to scale changes
	bWantsOnUpdateTransform = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/UXTools/Pointers/Meshes/SM_UnitQuad"));
	check(MeshFinder.Object);
	SetStaticMesh(MeshFinder.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/UXTools/Pointers/Materials/M_RingCursor"));
	check(MaterialFinder.Object);
	SetMaterial(0, MaterialFinder.Object);
}

void UUxtRingCursorComponent::OnRegister()
{
	Super::OnRegister();

	MaterialInstance = CreateDynamicMaterialInstance(0, GetMaterial(0));

	// Intialize radius from current scale
	OnUpdateTransform(EUpdateTransformFlags::None);

	// Update material parameters
	SetRingThickness(RingThickness);
	SetBorderThickness(BorderThickness);
	SetRingColor(RingColor);
	SetBorderColor(BorderColor);
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

void UUxtRingCursorComponent::SetRingThickness(float NewRingThickness)
{
	static FName InnerRadiusParameter = "InnerRadius";

	if (bUseAbsoluteThickness)
	{
		MaterialInstance->SetScalarParameterValue(InnerRadiusParameter, 1.0f - (NewRingThickness / Radius));
	}
	else
	{
		MaterialInstance->SetScalarParameterValue(InnerRadiusParameter, 1.0f - NewRingThickness);
	}
	
	RingThickness = NewRingThickness;
}

void UUxtRingCursorComponent::SetBorderThickness(float NewBorderThickness)
{
	static FName BorderThicknessParameter = "BorderThickness";

	if (bUseAbsoluteThickness)
	{
		MaterialInstance->SetScalarParameterValue(BorderThicknessParameter, NewBorderThickness / Radius);
	}
	else
	{
		MaterialInstance->SetScalarParameterValue(BorderThicknessParameter, NewBorderThickness);
	}

	BorderThickness = NewBorderThickness;
}

void UUxtRingCursorComponent::SetUseAbsoluteThickness(bool bNewUsingAboluteThickness)
{
	if (bNewUsingAboluteThickness != bUseAbsoluteThickness)
	{
		bUseAbsoluteThickness = bNewUsingAboluteThickness;
		SetRingThickness(RingThickness);
		SetBorderThickness(BorderThickness);
	}
}

void UUxtRingCursorComponent::SetRingColor(FColor NewRingColor)
{
	static FName RingColorParameter = "RingColor";
	MaterialInstance->SetVectorParameterValue(RingColorParameter, NewRingColor);
	RingColor = NewRingColor;
}

void UUxtRingCursorComponent::SetBorderColor(FColor NewBorderColor)
{
	static FName BorderColorParameter = "BorderColor";
	MaterialInstance->SetVectorParameterValue(BorderColorParameter, NewBorderColor);
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

	static FName RadiusParameter = "InvRadius";
	MaterialInstance->SetScalarParameterValue(RadiusParameter, 1.0f / Radius);

	if (bUseAbsoluteThickness)
	{
		SetRingThickness(RingThickness);
		SetBorderThickness(BorderThickness);
	}
}