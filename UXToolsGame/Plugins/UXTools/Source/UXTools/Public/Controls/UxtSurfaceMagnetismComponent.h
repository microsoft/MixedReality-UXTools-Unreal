// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/BoxComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"

#include "UxtSurfaceMagnetismComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtOnMagnetismStarted, UUxtSurfaceMagnetismComponent*, SurfaceMagnetism);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtOnMagnetismEnded, UUxtSurfaceMagnetismComponent*, SurfaceMagnetism);

/**
 * Surface magnetism is a control used to manipulate objects at a distance. The control allows you to grab an object using far interaction.
 * After that, the object will move to where the far beam intersects with collidable surfaces.
 */
UCLASS(ClassGroup = ("UXTools - Experimental"), HideCategories = (Shape), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtSurfaceMagnetismComponent
	: public UActorComponent
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUxtSurfaceMagnetismComponent();

	/** Get the current grabbed state of the component */
	UFUNCTION(BlueprintCallable, Category = "Uxt Surface Magnetism - Experimental")
	bool IsGrabbed() const { return IsActive(); }

	//
	// Events

	/** Event raised when Magnetism is activated */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Surface Magnetism - Experimental")
	FUxtOnMagnetismStarted OnMagnetismStarted;

	/** Event raised when Magnetism is deactivated */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Surface Magnetism - Experimental")
	FUxtOnMagnetismEnded OnMagnetismEnded;

	//
	// Getters and setters

	/** Get the component to transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Surface Magnetism - Experimental")
	UPrimitiveComponent* GetTargetComponent() const;

	/** Set the component to transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Surface Magnetism - Experimental")
	void SetTargetComponent(UPrimitiveComponent* Target);

protected:
	//
	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void BeginPlay() override;

	//
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:
	/** Internal function to set owning actors location based on a trace */
	void TraceAndSetActorLocation(FVector Start, FVector End, float DeltaTime);

public:
	/** How far to trace the world for a surface to stick to */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	float TraceDistance = 2000.0f;

	/** Set or interpolate position */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	bool bSmoothPosition = true;

	/** How fast to interpolate position */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	float PositionInterpValue = 8.0f;

	/** Set or interpolate rotation */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	bool bSmoothRotation = true;

	/** How fast to interpolate rotation */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	float RotationInterpValue = 8.0f;

	/** Offset target based on impact normal */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	float ImpactNormalOffset = 0.0f;

	/** Offset target back along traced ray */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	float TraceRayOffset = 0.0f;

	/** Set collision channel to trace for placement */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Whether the orientation of the object should pitch or roll */
	UPROPERTY(EditAnywhere, Category = "Uxt Surface Magnetism - Experimental")
	bool bKeepOrientationVertical = false;

private:
	/** The component to transform, defaults to the first primitive component if not specified */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Surface Magnetism - Experimental",
		meta = (UseComponentPicker, AllowedClasses = "UPrimitiveComponent"))
	FComponentReference TargetComponent;

	/** Far pointer in use */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	/** Stored target point (used if we are smoothing location) */
	FVector TargetLocation;

	/** Stored target rotation (used if we are smoothing rotation) */
	FRotator TargetRotation;
};
