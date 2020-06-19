// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "UxtBackPlateComponent.generated.h"

/**
 * Static mesh which automatically updates scale dependent material properties for back plates.
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

	UFUNCTION(BlueprintGetter)
	UMaterialInterface* GetBackPlateMaterial() const;
	UFUNCTION(BlueprintSetter)
	void SetBackPlateMaterial(UMaterialInterface* NewMaterial);

protected:

	virtual void OnRegister() override;
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

	/** Applies updated material parameters and instantiates a dynamic material property if necessary. */
	virtual void UpdateMaterialParameters();

	/** The default back plate material. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetBackPlateMaterial", BlueprintSetter = "SetBackPlateMaterial", Category = "Back Plate")
	UMaterialInterface* Material = nullptr;

	/** Handle to any dynamic material this component instantiates due to material parameter changes. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstance = nullptr;
};
