// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/StaticMeshComponent.h"
#include "Controls/UxtUIElementComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtGrabHandler.h"
#include "Interactions/UxtGrabTarget.h"

#include "UxtPinchSliderComponent.generated.h"

/**
 * Slider states.
 */
UENUM(BlueprintType)
enum class EUxtSliderState : uint8
{
	/** Slider is ready for interaction. */
	Default,
	/** Slider is being focused. */
	Focused,
	/** Slider is being grabbed. */
	Grabbed,
	/** Slider is disabled. */
	Disabled,
};

//
// Event delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtPinchSliderUpdateStateDelegate, UUxtPinchSliderComponent*, Slider, EUxtSliderState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtPinchSliderBeginFocusDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer, bool, bWasAlreadyFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtPinchSliderUpdateFocusDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FUxtPinchSliderEndFocusDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer, bool, bIsStillFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtPinchSliderBeginGrabDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtPinchSliderUpdateValueDelegate, UUxtPinchSliderComponent*, Slider, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FUxtPinchSliderEndGrabDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderEnableDelegate, UUxtPinchSliderComponent*, Slider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderDisableDelegate, UUxtPinchSliderComponent*, Slider);

/**
 * A slider that can be moved by grabbing / pinching a slider thumb.
 *
 * The thumb visuals must be set using the 'Visuals' property.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtPinchSliderComponent
	: public UUxtUIElementComponent
	, public IUxtGrabTarget
	, public IUxtGrabHandler
	, public IUxtFarTarget
	, public IUxtFarHandler
{
	GENERATED_BODY()

public:
	//
	// Public interface.

	/** Get the current state of the slider. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pinch Slider")
	EUxtSliderState GetState() const { return State; }

	/** Set if the slider is enabled. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pinch Slider")
	void SetEnabled(bool bEnabled);

	/** Get the static mesh representing the thumb visuals. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pinch Slider")
	UStaticMeshComponent* GetVisuals() const { return Cast<UStaticMeshComponent>(Visuals.GetComponent(GetOwner())); }

	/** Set the static mesh representing the thumb visuals. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Pinch Slider")
	void SetVisuals(UStaticMeshComponent* NewVisuals);

	/** Set the thumb visuals using a component reference, this is necessary if the visuals will be serialized. */
	void SetVisuals(const FComponentReference& NewVisuals);

	//
	// Getters and setters.

	// Value.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetValue() const { return Value; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetValue(float NewValue);

	// Track length.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetTrackLength() const { return TrackLength; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetTrackLength(float NewTrackLength);

	// Value lower bound.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetValueLowerBound() const { return ValueLowerBound; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetValueLowerBound(float NewLowerBound);

	// Value upper bound.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetValueUpperBound() const { return ValueUpperBound; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetValueUpperBound(float NewUpperBound);

	// Use stepped movement.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	bool GetUseSteppedMovement() const { return bUseSteppedMovement; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetUseSteppedMovement(bool bNewUseSteppedMovement);

	// Step value.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	int GetNumSteps() const { return NumSteps; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetNumSteps(int NewNumSteps);

	// Smoothing.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetSmoothing() const { return Smoothing; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetSmoothing(float NewSmoothing);

	// Collision profile.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	FName GetCollisionProfile() const { return CollisionProfile; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetCollisionProfile(FName NewCollisionProfile);

	//
	// Events

	/** Event raised when slider changes state. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderUpdateStateDelegate OnUpdateState;

	/** Event raised when a pointer starts focusing the slider. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderBeginFocusDelegate OnBeginFocus;

	/** Event raised when a focusing pointer updates. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderUpdateFocusDelegate OnUpdateFocus;

	/** Event raised when a pointer stops focusing the slider. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderEndFocusDelegate OnEndFocus;

	/** Event raised when slider is grabbed. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderBeginGrabDelegate OnBeginGrab;

	/** Event raised when slider's value changes. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderUpdateValueDelegate OnUpdateValue;

	/** Event raised when slider is released. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderEndGrabDelegate OnEndGrab;

	/** Event raised when slider is enabled. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderEnableDelegate OnEnable;

	/** Event raised when slider is disabled. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Pinch Slider")
	FUxtPinchSliderDisableDelegate OnDisable;

protected:
	//
	// UActorComponent interface.

	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//
	// IUxtGrabTarget interface.
	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtGrabHandler interface
	virtual bool CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface.
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnUpdatedFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;

private:
	//
	// Common interaction logic.

	void BeginFocus(UUxtPointerComponent* Pointer);
	void UpdateFocus(UUxtPointerComponent* Pointer);
	void EndFocus(UUxtPointerComponent* Pointer);
	void BeginGrab(UUxtPointerComponent* Pointer);
	void UpdateGrab(FVector DeltaPosition);
	void EndGrab(UUxtPointerComponent* Pointer);

	//
	// Internal state updates.

	void SetState(EUxtSliderState NewState);
	void ConfigureBoxComponent();
	void UpdateVisuals();

	//
	// Configurable properties.

	/** The thumb visuals. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"))
	FComponentReference Visuals;

	/** The slider's position on the track, between 0-1. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetValue, BlueprintSetter = SetValue,
		meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float Value = 0.0f;

	/** The length of the slider's track. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetTrackLength, BlueprintSetter = SetTrackLength,
		meta = (ClampMin = 0.0f))
	float TrackLength = 50.0f;

	/** The lower bound for the slider, between 0-1. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", AdvancedDisplay, BlueprintGetter = GetValueLowerBound,
		BlueprintSetter = SetValueLowerBound, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float ValueLowerBound = 0.0f;

	/** The upper bound for the slider, between 0-1. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", AdvancedDisplay, BlueprintGetter = GetValueUpperBound,
		BlueprintSetter = SetValueUpperBound, meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float ValueUpperBound = 1.0f;

	/** Should the slider use stepped or smooth movement. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", AdvancedDisplay, BlueprintGetter = GetUseSteppedMovement,
		BlueprintSetter = SetUseSteppedMovement)
	bool bUseSteppedMovement = false;

	/** The number of steps for stepped slider movement. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", AdvancedDisplay, BlueprintGetter = GetNumSteps, BlueprintSetter = SetNumSteps,
		meta = (ClampMin = 2, EditCondition = "bUseSteppedMovement"))
	int NumSteps = 5;

	/** The motion smoothing to apply to the slider. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", AdvancedDisplay, BlueprintGetter = GetSmoothing, BlueprintSetter = SetSmoothing,
		meta = (ClampMin = 0.0f, EditCondition = "!bUseSteppedMovement"))
	float Smoothing = 10.0f;

	/** The collision profile used by the slider thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", AdvancedDisplay, BlueprintGetter = GetCollisionProfile,
		BlueprintSetter = SetCollisionProfile)
	FName CollisionProfile = TEXT("UI");

	//
	// Private properties.

	/** The box collider used for grabbing the slider. */
	UPROPERTY(Transient)
	class UBoxComponent* BoxComponent;

	/** The current state. */
	EUxtSliderState State = EUxtSliderState::Default;

	/** The position of the hand when the slider was grabbed in world space. */
	FVector HandStartPosition;

	/** The position of the slider when it was grabbed in local space. */
	float SliderStartPosition;

	/** The pointers currently focusing the slider. */
	TArray<UUxtPointerComponent*> FocusingPointers;

	/** The pointer currently grabbing the slider. */
	UUxtPointerComponent* GrabPointer;
};
