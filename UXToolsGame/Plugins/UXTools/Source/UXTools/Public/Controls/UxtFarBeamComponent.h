// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/SplineMeshComponent.h"

#include "UxtFarBeamComponent.generated.h"

class UUxtFarPointerComponent;

/**
 * When added to an actor with a far pointer, this component displays a beam from the pointer ray start to the current hit point.
 */
UCLASS(ClassGroup = "UXTools", HideCategories = SplineMeshComponent, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFarBeamComponent : public USplineMeshComponent
{
	GENERATED_BODY()

public:
	UUxtFarBeamComponent();

	//
	// UActorComponent interface

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Using this function you can change the beam material at runtime
	UFUNCTION(BlueprintCallable, Category = "Uxt Far Beam")
	void SetBeamMaterial(UMaterial* NewMaterial);

	/** Distance over the hit surface to place beam end at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Far Beam")
	float HoverDistance = 0.5f;

private:
	UFUNCTION(Category = "Uxt Far Beam")
	void OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer);

	UFUNCTION(Category = "Uxt Far Beam")
	void OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer);

	/** Dynamic Material to pass internal state to shader */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MID;

	/** Far pointer in use. */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	/** Should we send grab to the material */
	bool BindGrab;
	/** Should we send spline length to the material */
	bool BindSplineLength;
};
