// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "UxtTapToPlaceComponent.generated.h"

UENUM(BlueprintType)
enum EUxtTapToPlaceOrientBehavior
{
	/** Billboard toward the camera */
	AlignToCamera UMETA(DisplayName = "FaceCamera"),
	/** Align to hit surface. If no hit surface, will face camera */
	AlignToSurface UMETA(DisplayName = "AlignToSurface"),
};

UENUM(BlueprintType)
enum EUxtTapToPlaceMode
{
	/** Place using look direction */
	Head,
	/** Place using the hand ray */
	Hand
};

//
// Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtTapToPlaceBeginFocusDelegate, UUxtTapToPlaceComponent*, TapToPlace, UUxtFarPointerComponent*, Pointer, bool, bWasAlreadyFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtTapToPlaceUpdateFocusDelegate, UUxtTapToPlaceComponent*, TapToPlace, UUxtFarPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtTapToPlaceEndFocusDelegate, UUxtTapToPlaceComponent*, TapToPlace, UUxtFarPointerComponent*, Pointer, bool, bIsStillFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtTapToPlaceBeginPlacingDelegate, UUxtTapToPlaceComponent*, TapToPlace);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtTapToPlaceEndPlacingDelegate, UUxtTapToPlaceComponent*, TapToPlace);

/**
 * Tap to place is a control used to transform objects at a distance. The control allows you to select an object
 * you wish to place using far interaction. After that, the object will be locked to your gaze and will be placed
 * against surfaces and other objects. Any subsequent far release will end placement, even if the pointer is not
 * pointing at the object being placed.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTapToPlaceComponent
	: public UActorComponent
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	UUxtTapToPlaceComponent();

	/** Get the component to transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Tap To Place")
	UPrimitiveComponent* GetTargetComponent() const;

	/** Set the component to transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Tap To Place")
	void SetTargetComponent(UPrimitiveComponent* Target);

	/** Start placement of the target component. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Tap To Place")
	void StartPlacement();

	/** End placement of the target component. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Tap To Place")
	void EndPlacement();

protected:
	//
	// UActorComponent interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

public:
	/** Event raised when a pointer starts focusing the placeable object. bWasAlreadyFocused indicates if the object was already focused by
	 * another pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tap To Place")
	FUxtTapToPlaceBeginFocusDelegate OnBeginFocus;

	/** Event raised when a focusing pointer updates. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tap To Place")
	FUxtTapToPlaceUpdateFocusDelegate OnUpdateFocus;

	/** Event raised when a pointer ends focusing the placeable object. bIsStillFocused indicates if the object is still focused by another
	 * pointer. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tap To Place")
	FUxtTapToPlaceEndFocusDelegate OnEndFocus;

	/** Event raised when a placeable object is selected. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tap To Place")
	FUxtTapToPlaceBeginPlacingDelegate OnBeginPlacing;

	/** Event raised when a placeable object is deselected and placed. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tap To Place")
	FUxtTapToPlaceEndPlacingDelegate OnEndPlacing;

	/** How the object is oriented against hit surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	TEnumAsByte<EUxtTapToPlaceOrientBehavior> OrientationType = EUxtTapToPlaceOrientBehavior::AlignToSurface;

	/** How the target should be placed, using head or far pointer. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tap To Place")
	TEnumAsByte<EUxtTapToPlaceMode> PlacementType = EUxtTapToPlaceMode::Head;

	/** Whether the orientation of the object should pitch or roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	bool KeepOrientationVertical = false;

	/** Distance to place the object at if no obstructing surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	float DefaultPlacementDistance = 150;

	/** Max distance to cast to when checking for obstructing surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	float MaxRaycastDistance = 2000;

	/** Trace channel for raycast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	/** Option to ignore interpolation between follow poses */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	bool bInterpolatePose = true;

	/** Rate at which its owner will move toward default distance when angular leashing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Tap To Place")
	float LerpTime = 0.1f;

private:
	/** The component to transform, defaults to the first primitive component if not specified */
	UPROPERTY(EditAnywhere, Category = "Uxt Tap To Place", meta = (UseComponentPicker, AllowedClasses = "PrimitiveComponent"))
	FComponentReference TargetComponent;

	bool bIsBeingPlaced = false;
	float SurfaceNormalOffset;
	int NumFocusedPointers = 0;

	/** Far pointer that initiated placement, if any. */
	TWeakObjectPtr<UUxtFarPointerComponent> FocusLockedPointerWeak;
};
