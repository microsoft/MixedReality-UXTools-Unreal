// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "UxtRingCursorComponent.generated.h"

class UMaterialInstanceDynamic;

/**
 * Displays a flat ring facing -X. The ring radius can be set directly or via the component scale.
 */
UCLASS( ClassGroup = UXTools, HideCategories = (StaticMesh, Materials), meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtRingCursorComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UUxtRingCursorComponent();

	UFUNCTION(BlueprintGetter)
	float GetRadius() const { return Radius; }
	UFUNCTION(BlueprintSetter)
	void SetRadius(float Radius);

	UFUNCTION(BlueprintGetter)
	float GetRingThickness() const { return RingThickness; }
	UFUNCTION(BlueprintSetter)
	void SetRingThickness(float NewRingThickness);

	UFUNCTION(BlueprintGetter)
	float GetBorderThickness() const { return BorderThickness; }
	UFUNCTION(BlueprintSetter)
	void SetBorderThickness(float NewBorderThickness);

	UFUNCTION(BlueprintGetter)
	bool GetUseAbsoluteThickness() const { return bUseAbsoluteThickness; }
	UFUNCTION(BlueprintSetter)
	void SetUseAbsoluteThickness(bool NewbUsingAbsoluteThickness);

	UFUNCTION(BlueprintGetter)
	FColor GetRingColor() const { return RingColor; }
	UFUNCTION(BlueprintSetter)
	void SetRingColor(FColor NewRingColor);

	UFUNCTION(BlueprintGetter)
	FColor GetBorderColor() const { return BorderColor; }
	UFUNCTION(BlueprintSetter)
	void SetBorderColor(FColor NewBorderColor);

protected:

	virtual void OnRegister() override;

	/** Used to update the radius in response to scale changes. */
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

	/** Ring thickness, including border, expressed as a fraction of the ring radius unless Use Absolute Thickness is enabled */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetRingThickness", BlueprintSetter = "SetRingThickness", Category = "Ring Cursor")
	float RingThickness = 0.65f;

	/** Border thickness expressed as a fraction of the ring radius unless Use Absolute Thickness is enabled */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetBorderThickness", BlueprintSetter = "SetBorderThickness", Category = "Ring Cursor")
	float BorderThickness = 0.08f;

	/* When set thickness values are taken to be in world units instead of relative to the ring radius. */
	UPROPERTY(EditAnywhere, BlueprintGetter = "GetUseAbsoluteThickness", BlueprintSetter = "SetUseAbsoluteThickness", Category = "Ring Cursor")
	bool bUseAbsoluteThickness = false;

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetRingColor", BlueprintSetter = "SetRingColor", Category = "Ring Cursor")
	FColor RingColor = FColor::White;

	UPROPERTY(EditAnywhere, BlueprintGetter = "GetBorderColor", BlueprintSetter = "SetBorderColor", Category = "Ring Cursor")
	FColor BorderColor = FColor::Black;

private:

	void SetRadius(float Radius, bool bUpdateScale);

	/** Dynamic instance of the ring material. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstance;

	UPROPERTY(Transient, BlueprintGetter = "GetRadius", BlueprintSetter = "SetRadius", Category = "Ring Cursor")
	float Radius;

	/** Used to ignore transform changes triggered from SetRadius(). */
	bool bSettingRadius = false;
};
