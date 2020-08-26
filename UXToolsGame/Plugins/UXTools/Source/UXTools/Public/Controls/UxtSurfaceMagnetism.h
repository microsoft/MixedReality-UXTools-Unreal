// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/BoxComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Interactions/UxtFarTarget.h"

#include "UxtSurfaceMagnetism.generated.h"

UENUM(BlueprintType)
enum class EUxtMagnetismType : uint8
{
	/** Magnetism by look direction */
	Head,
	/** magnetism by hand ray */
	Hand
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtOnMagnetismStarted, UUxtSurfaceMagnetism*, SurfaceMagnetism);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtOnMagnetismEnded, UUxtSurfaceMagnetism*, SurfaceMagnetism);

UCLASS(ClassGroup = ("UXTools - Experimental"), HideCategories = (Shape), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtSurfaceMagnetism
	: public UBoxComponent
	, public IUxtFarTarget
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUxtSurfaceMagnetism();

	/** Set collision profile for the box target*/
	UFUNCTION(BlueprintCallable, Category = "Surface Magnetism - Experimental")
	void SetCollisionProfile(FName Profile);

	/** Get the current grabbed state of the component */
	UFUNCTION(BlueprintCallable, Category = "Surface Magnetism - Experimental")
	bool IsGrabbed() const { return IsComponentTickEnabled(); }

#if WITH_EDITORONLY_DATA
	/** Editor update function - called by UE4 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//
	// Events

	/** Event raised when Magnetism is activated */
	UPROPERTY(BlueprintAssignable, Category = "Surface Magnetism - Experimental")
	FUxtOnMagnetismStarted OnMagnetismStarted;

	/** Event raised when Magnetism is deactivated */
	UPROPERTY(BlueprintAssignable, Category = "Surface Magnetism - Experimental")
	FUxtOnMagnetismEnded OnMagnetismEnded;

	//
	// Getters and setters

	UFUNCTION(BlueprintGetter)
	EUxtMagnetismType GetMagnetismType() const { return MagnetismType; }
	UFUNCTION(BlueprintSetter)
	void SetMagnetismType(EUxtMagnetismType NewType) { MagnetismType = NewType; }

	UFUNCTION(BlueprintGetter)
	float GetTraceDistance() const { return TraceDistance; }
	UFUNCTION(BlueprintSetter)
	void SetTraceDistance(float Distance) { TraceDistance = Distance; }

	UFUNCTION(BlueprintGetter)
	bool IsPositionSmoothed() const { return bSmoothPosition; }
	UFUNCTION(BlueprintSetter)
	void SetSmoothPosition(bool IsSmoothed) { bSmoothPosition = IsSmoothed; }

	UFUNCTION(BlueprintGetter)
	bool IsRotationSmoothed() const { return bSmoothRotation; }
	UFUNCTION(BlueprintSetter)
	void SetSmoothRotation(bool IsSmoothed) { bSmoothRotation = IsSmoothed; }

	UFUNCTION(BlueprintGetter)
	float GetPositionInterpValue() const { return PositionInterpValue; }
	UFUNCTION(BlueprintSetter)
	void SetPositionInterpValue(float NewValue) { PositionInterpValue = NewValue; }

	UFUNCTION(BlueprintGetter)
	float GetRotationInterpValue() const { return RotationInterpValue; }
	UFUNCTION(BlueprintSetter)
	void SetRotationInterpValue(float NewValue) { RotationInterpValue = NewValue; }

	UFUNCTION(BlueprintGetter)
	float GetImpactNormalOffset() const { return ImpactNormalOffset; }
	UFUNCTION(BlueprintSetter)
	void SetImpactNormalOffset(float NewValue) { ImpactNormalOffset = NewValue; }

	UFUNCTION(BlueprintGetter)
	float GetTraceRayOffset() const { return TraceRayOffset; }
	UFUNCTION(BlueprintSetter)
	void SetTraceRayOffset(float NewValue) { TraceRayOffset = NewValue; }

	UFUNCTION(BlueprintGetter)
	TEnumAsByte<ECollisionChannel> GetTraceChannel() const { return TraceChannel; }
	UFUNCTION(BlueprintSetter)
	void SetTraceChannel(TEnumAsByte<ECollisionChannel> NewValue) { TraceChannel = NewValue; }

	UFUNCTION(BlueprintGetter)
	bool IsAutoBounds() const { return bAutoBounds; }
	UFUNCTION(BlueprintSetter)
	void SetAutoBounds(bool DoAutoBounds);

	UFUNCTION(BlueprintGetter)
	FVector GetBoxBounds() const { return BoxBounds; }
	UFUNCTION(BlueprintSetter)
	void SetBoxBounds(FVector NewBounds);

	UFUNCTION(BlueprintGetter)
	bool IsOnlyYawEnabled() const { return bOnlyYawEnabled; }
	UFUNCTION(BlueprintSetter)
	void SetOnlyYawEnabled(bool UseYawOnly);

protected:
	//
	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentCreated() override;
	virtual void BeginPlay() override;
	//
	// IUxtFarTarget interface
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) override;

private:
	/** Internal function to set owning actors location based on a trace */
	void TraceAndSetActorLocation(FVector Start, FVector End, float DeltaTime);

	// internal function to update interactable area
	void SetupBounds();

	/** The type of magnetism - head or pointer based */
	UPROPERTY(
		EditAnywhere, DisplayName = "MagnetismType", BlueprintGetter = "GetMagnetismType", BlueprintSetter = "SetMagnetismType",
		Category = "Surface Magnetism - Experimental")
	EUxtMagnetismType MagnetismType;

	/** How far to trace the world for a surface to stick to */
	UPROPERTY(
		EditAnywhere, DisplayName = "TraceDistance", BlueprintGetter = "GetTraceDistance", BlueprintSetter = "SetTraceDistance",
		Category = "Surface Magnetism - Experimental")
	float TraceDistance;

	/** Set or interpolate position  */
	UPROPERTY(
		EditAnywhere, DisplayName = "SmoothPosition", BlueprintGetter = "IsPositionSmoothed", BlueprintSetter = "SetSmoothPosition",
		Category = "Surface Magnetism - Experimental")
	bool bSmoothPosition;

	/** How fast to interpolate position  */
	UPROPERTY(
		EditAnywhere, DisplayName = "PositionInterpValue", BlueprintGetter = "GetPositionInterpValue",
		BlueprintSetter = "SetPositionInterpValue", Category = "Surface Magnetism - Experimental")
	float PositionInterpValue;

	/** Set or interpolate rotation   */
	UPROPERTY(
		EditAnywhere, DisplayName = "SmoothRotation", BlueprintGetter = "IsRotationSmoothed", BlueprintSetter = "SetSmoothRotation",
		Category = "Surface Magnetism - Experimental")
	bool bSmoothRotation;

	/** How fast to interpolate rotation */
	UPROPERTY(
		EditAnywhere, DisplayName = "RotationInterpValue", BlueprintGetter = "GetRotationInterpValue",
		BlueprintSetter = "SetRotationInterpValue", Category = "Surface Magnetism - Experimental")
	float RotationInterpValue;

	/** Offset target based on impact normal */
	UPROPERTY(
		EditAnywhere, DisplayName = "ImpactNormalOffset", BlueprintGetter = "GetImpactNormalOffset",
		BlueprintSetter = "SetImpactNormalOffset", Category = "Surface Magnetism - Experimental")
	float ImpactNormalOffset;

	/** Offset target back along traced ray */
	UPROPERTY(
		EditAnywhere, DisplayName = "TraceRayOffset", BlueprintGetter = "GetTraceRayOffset", BlueprintSetter = "SetTraceRayOffset",
		Category = "Surface Magnetism - Experimental")
	float TraceRayOffset;
	/** Set collision channel to trace for placement */
	UPROPERTY(
		EditAnywhere, DisplayName = "TraceChannel", BlueprintGetter = "GetTraceChannel", BlueprintSetter = "SetTraceChannel",
		Category = "Surface Magnetism - Experimental")
	TEnumAsByte<ECollisionChannel> TraceChannel;

	/** Set collision area automatically to owning actors bounds  */
	UPROPERTY(
		EditAnywhere, DisplayName = "AutoBounds", BlueprintGetter = "IsAutoBounds", BlueprintSetter = "SetAutoBounds",
		Category = "Surface Magnetism - Experimental")
	bool bAutoBounds;

	/** Specify collision area bounds, AutoBounds must be off */
	UPROPERTY(
		EditAnywhere, DisplayName = "BoxBounds", BlueprintGetter = "GetBoxBounds", BlueprintSetter = "SetBoxBounds",
		Category = "Surface Magnetism - Experimental")
	FVector BoxBounds;

	/** Set collision area automatically to owning actors bounds  */
	UPROPERTY(
		EditAnywhere, DisplayName = "OnlyEnableYaw", BlueprintGetter = "IsOnlyYawEnabled", BlueprintSetter = "SetOnlyYawEnabled",
		Category = "Surface Magnetism - Experimental")
	bool bOnlyYawEnabled;

	/** Collision profile used by the box target*/
	UPROPERTY(EditAnywhere, Category = "Surface Magnetism - Experimental")
	FName CollisionProfile;

	/** Far pointer in use. */
	TWeakObjectPtr<UUxtFarPointerComponent> FarPointerWeak;

	/** Stored target point (used if we are smoothing location) */
	FVector HitLocation;

	/** Stored target rotation (used if we are smoothing rotation)*/
	FRotator HitRotation;

	/** Interaction has ended  (used if we are smoothing location or rotation)*/
	bool bInteractionHalted;
};
