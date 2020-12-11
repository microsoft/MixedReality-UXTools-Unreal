// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/StaticMeshComponent.h"

#include "UxtRingCursorComponent.generated.h"

class UMaterialInstanceDynamic;

/**
 * Displays a flat ring facing -X. The ring radius can be set directly or via the component scale.
 */
UCLASS(ClassGroup = "UXTools", HideCategories = (StaticMesh, Materials), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtRingCursorComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUxtRingCursorComponent();

	UFUNCTION(BlueprintGetter, Category = "Uxt Ring Cursor")
	float GetRadius() const { return Radius; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Ring Cursor")
	void SetRadius(float Radius);

	UFUNCTION(BlueprintGetter, Category = "Uxt Ring Cursor")
	FColor GetRingColor() const { return RingColor; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Ring Cursor")
	void SetRingColor(FColor NewRingColor);

	UFUNCTION(BlueprintGetter, Category = "Uxt Ring Cursor")
	FColor GetBorderColor() const { return BorderColor; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Ring Cursor")
	void SetBorderColor(FColor NewBorderColor);

protected:
	virtual void OnRegister() override;

	/** Used to update the radius in response to scale changes. */
	virtual void OnUpdateTransform(EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport = ETeleportType::None) override;

	UPROPERTY(EditAnywhere, Category = "Uxt Ring Cursor", BlueprintGetter = "GetRingColor", BlueprintSetter = "SetRingColor")
	FColor RingColor = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Uxt Ring Cursor", BlueprintGetter = "GetBorderColor", BlueprintSetter = "SetBorderColor")
	FColor BorderColor = FColor::Black;

	/** Cursor meshes. Swapping dynamically on the fly depends on its state. **/
	UPROPERTY(Transient)
	UStaticMesh* FocusMesh;
	UPROPERTY(Transient)
	UStaticMesh* PressMesh;

private:
	void SetRadius(float Radius, bool bUpdateScale);

	/** Dynamic instance of the ring material. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstanceRing;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstanceBorder;

	UPROPERTY(Transient, Category = "Uxt Ring Cursor", BlueprintGetter = "GetRadius", BlueprintSetter = "SetRadius")
	float Radius;

	/** Used to ignore transform changes triggered from SetRadius(). */
	bool bSettingRadius = false;
};
