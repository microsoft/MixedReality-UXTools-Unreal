// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBackPlateComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UUxtBackPlateComponent::UUxtBackPlateComponent()
{
	SetCastShadow(false);

	// Used to update material parameters in response to scale changes.
	bWantsOnUpdateTransform = true;

	// Apply default assets.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MeshFinder(TEXT("/UXTools/Models/SM_BackPlateRoundedThick_4"));
	check(MeshFinder.Object);
	SetStaticMesh(MeshFinder.Object);

	// Bug, the FObjectFinder does not pull in files referenced by UMaterialExpressionCustom::IncludeFilePaths and fails to compile during cooking.
	//static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/UXTools/Materials/MI_HoloLens2BackPlate"));
	//check(MaterialFinder.Object);
	//Material = MaterialFinder.Object;
	//SetMaterial(0, Material);

	// Initialize the mesh to point down the +X axis with the default scale.
	SetRelativeRotation(FRotator(90, 0, 0));
	SetRelativeScale3D(FVector(3.2f, 3.2f, 1.6f));
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

void UUxtBackPlateComponent::OnRegister()
{
	Super::OnRegister();

	UpdateMaterialParameters();
}

void UUxtBackPlateComponent::OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	UpdateMaterialParameters();
}

void UUxtBackPlateComponent::UpdateMaterialParameters()
{
	if (Material == nullptr)
	{
		SetMaterial(0, nullptr);
		MaterialInstance = nullptr;
		return;
	}

	const float Width = GetRelativeScale3D().Y;

	if (Width == 0.0f)
	{
		return;
	}

	// The default material assumes a width of 32mm. If the width is 32mm then just use the default material and
	// destroy any instances.
	if (FMath::IsNearlyEqual(Width, 3.2f))
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
