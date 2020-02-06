// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "UxtFingerCursorComponent.generated.h"

class UUxtTouchPointer;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * When added to an actor with a touch pointer, this component displays a ring cursor oriented towards the pointer target and 
 * scaled according to the distance.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UXTOOLS_API UUxtFingerCursorComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UUxtFingerCursorComponent();

protected:

	//
	// UActorComponent interface

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	/** Maximum distance to the pointer target at which the cursor should be displayed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Cursor")
	float MaxDistanceToTarget = 20.0f;

	/** Outer radius of the cursor ring at the maximum distance to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Cursor")
	float MaxOuterRadius = 0.75f;

	/** Outer radius of the cursor ring when in contact with the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Cursor")
	float MinOuterRadius = 0.32f;

private:

	/** Material used to display the cursor ring. Must derive from the default cursor ring material. */
	UPROPERTY(EditAnywhere, Category = "Finger Cursor")
	UMaterialInterface* RingMaterial;

	/** Distance at which the cursor starts to align with touchable surfaces. */
	UPROPERTY(EditAnywhere, Category = "Finger Cursor")
	float AlignWithSurfaceDistance = 10.0f;

	/** Mesh component created to display the cursor. */
	UStaticMeshComponent* MeshComponent;

	/** Dynamic instance of the ring material. */
	UMaterialInstanceDynamic* MaterialInstance;

	/** Touch pointer in use. */
	TWeakObjectPtr<UUxtTouchPointer> TouchPointerWeak;
};
