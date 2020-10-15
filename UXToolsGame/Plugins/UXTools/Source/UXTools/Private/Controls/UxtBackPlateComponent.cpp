// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBackPlateComponent.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

const float DefaultBackPlateDepth = 1.6f;
const float DefaultBackPlateSize = 3.2f;

// Specifies a rotation of 90 degrees along the x-axis to apply to the mesh component bounds.
// Ideally we would do perform this rotation at model import time, but the back plate shader assumes 'Y' is up
// so we also perform the same rotation in the vertex shader to avoid having to completely re-author the shader.
const FRotator ZUpRotation(90, 0, 0);

UUxtBackPlateComponent::UUxtBackPlateComponent()
{
	SetCastShadow(false);

	// Used to update material parameters in response to scale changes.
	bWantsOnUpdateTransform = true;

	// Apply default assets.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/UXTools/Models/SM_BackPlateRoundedThick_4"));
	check(MeshFinder.Object);
	SetStaticMesh(MeshFinder.Object);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/UXTools/Materials/MI_HoloLens2BackPlate"));
	check(MaterialFinder.Object);
	Material = MaterialFinder.Object;
	SetMaterial(0, Material);

	// Set the default backplate scale to 16x32x32mm.
	SetRelativeScale3D(FVector(DefaultBackPlateDepth, DefaultBackPlateSize, DefaultBackPlateSize));
}

#if WITH_EDITOR
void UUxtBackPlateComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UUxtBackPlateComponent, Material))
	{
		UpdateMaterialParameters();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

UMaterialInterface* UUxtBackPlateComponent::GetBackPlateMaterial() const
{
	return (MaterialInstance != nullptr) ? MaterialInstance : Material;
}

void UUxtBackPlateComponent::SetBackPlateMaterial(UMaterialInterface* NewMaterial)
{
	Material = NewMaterial;

	UpdateMaterialParameters();
}

float UUxtBackPlateComponent::GetDefaultBackPlateDepth()
{
	return DefaultBackPlateDepth;
}

float UUxtBackPlateComponent::GetDefaultBackPlateSize()
{
	return DefaultBackPlateSize;
}

void UUxtBackPlateComponent::OnRegister()
{
	Super::OnRegister();

	UpdateMaterialParameters();
}

void UUxtBackPlateComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	Super::OnUpdateTransform(UpdateTransformFlags, Teleport);

	UpdateMaterialParameters();
}

FBoxSphereBounds UUxtBackPlateComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	// Rotate the local bounds by ZUpRotation to ensure the bounds fit correctly.
	FTransform RotatedLocalToWorld = LocalToWorld;
	RotatedLocalToWorld = FTransform(ZUpRotation) * RotatedLocalToWorld;
	// Note, scale needs to be rotated manually because multiplying FTransforms only scales the scale.
	RotatedLocalToWorld.SetScale3D(ZUpRotation.RotateVector(RotatedLocalToWorld.GetScale3D()));

	return Super::CalcBounds(RotatedLocalToWorld);
}

void UUxtBackPlateComponent::UpdateMaterialParameters()
{
	if (Material == nullptr)
	{
		SetMaterial(0, nullptr);
		MaterialInstance = nullptr;
		return;
	}

	const float Width = GetComponentScale().Y;

	if (Width == 0.0f)
	{
		return;
	}

	// The default material assumes a width of 32mm. If the width is 32mm then just use the default material and
	// destroy any instances.
	if (FMath::IsNearlyEqual(Width, DefaultBackPlateSize))
	{
		SetMaterial(0, Material);
		MaterialInstance = nullptr;
	}
	else
	{
		if (MaterialInstance == nullptr || (MaterialInstance->Parent != Material))
		{
			MaterialInstance = CreateDynamicMaterialInstance(0, Material);
			SetMaterial(0, MaterialInstance);
		}

		// Values derived from the MI_HoloLens2BackPlate defaults.
		static const FName RadiusName = "Radius";
		const float Radius = (12.8f / Width) / 100.0f;
		MaterialInstance->SetScalarParameterValue(RadiusName, Radius);

		static const FName LineWidthName = "Line_Width";
		const float LineWidth = (0.00005f / Width) * 1000.0f;
		MaterialInstance->SetScalarParameterValue(LineWidthName, LineWidth);
	}
}
