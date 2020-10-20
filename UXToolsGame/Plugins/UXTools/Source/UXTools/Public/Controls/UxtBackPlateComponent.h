// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/StaticMeshComponent.h"

#include "UxtBackPlateComponent.generated.h"

/**
 * Static mesh which automatically updates scale dependent material properties for back plates.
 * Note, this component also adjusts the bounds of the static mesh to ensure calculations happening
 * in the vertex shader are accounted for. Please see ZUpRotation for more information.
 */
UCLASS(ClassGroup = UXTools, HideCategories = (Materials), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtBackPlateComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUxtBackPlateComponent();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION(BlueprintGetter, Category = "Back Plate")
	UMaterialInterface* GetBackPlateMaterial() const;

	UFUNCTION(BlueprintSetter, Category = "Back Plate")
	void SetBackPlateMaterial(UMaterialInterface* NewMaterial);

	/** Gets the default depth a back plate should be scaled to along the x-axis. */
	UFUNCTION(BlueprintCallable, Category = "Back Plate")
	static float GetDefaultBackPlateDepth();

	/** Gets the default back plate scale along the the y and z axes. */
	UFUNCTION(BlueprintCallable, Category = "Back Plate")
	static float GetDefaultBackPlateSize();

protected:
	//
	// UActorComponent interface

	virtual void OnRegister() override;

	//
	// USceneComponent interface

	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	/** Applies updated material parameters and instantiates a dynamic material property if necessary. */
	virtual void UpdateMaterialParameters();

	/** The current back plate material. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetBackPlateMaterial", BlueprintSetter = "SetBackPlateMaterial", Category = "Back Plate")
	UMaterialInterface* Material = nullptr;

	/** Handle to any dynamic material this component instantiates due to material parameter changes. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstance = nullptr;
};
