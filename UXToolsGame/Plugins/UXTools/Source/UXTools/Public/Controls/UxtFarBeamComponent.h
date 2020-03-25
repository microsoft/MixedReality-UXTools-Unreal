// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "UxtFarBeamComponent.generated.h"

class UUxtFarPointerComponent;
class UMaterialInterface;
class APlayerCameraManager;

/**
 * When added to an actor with a far pointer, this component displays a beam from the pointer ray start to the current hit point.
 */
UCLASS(ClassGroup = UXTools, meta=(BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFarBeamComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:

	UUxtFarBeamComponent();

	//
	// UActorComponent interface

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Distance over the hit surface to place beam end at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Beam")
	float HoverDistance = 0.5f;

private:

	UFUNCTION()
	void OnFarPointerEnabled(UUxtFarPointerComponent* FarPointer);

	UFUNCTION()
	void OnFarPointerDisabled(UUxtFarPointerComponent* FarPointer);

private:

	/** Far pointer in use. */	
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;
};
