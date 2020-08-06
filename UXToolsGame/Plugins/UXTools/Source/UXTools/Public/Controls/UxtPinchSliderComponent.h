// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Controls/UxtUIElementComponent.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtFarTarget.h"
#include "Input/UxtPointerComponent.h"
#include "UxtPinchSliderComponent.generated.h"

UENUM(BlueprintType)
enum class EUxtSliderState : uint8
{
	/** Slider is not interacting */
	Default,
	/** Slider is in focus state */
	Focus,
	/** Slider is in focus state */
	Grab,
	/** Slider is in disabled state */
	Disabled,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtPinchSliderUpdateValueDelegate, UUxtPinchSliderComponent*, Slider, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtPinchSliderBeginInteractionDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtPinchSliderEndInteractionDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUxtPinchSliderBeginFocusDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer, bool, bWasAlreadyFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUxtPinchSliderUpdateFocusDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUxtPinchSliderEndFocusDelegate, UUxtPinchSliderComponent*, Slider, UUxtPointerComponent*, Pointer, bool, bIsStillFocused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (FUxtPinchSliderUpdateStateDelegate, EUxtSliderState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderEnabledDelegate, UUxtPinchSliderComponent*, Slider);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtPinchSliderDisabledDelegate, UUxtPinchSliderComponent*, Slider);

/**
 * Component that implements a thumb slider UI and logic.
 */
UCLASS( ClassGroup = UXTools, meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtPinchSliderComponent : public UUxtUIElementComponent, public IUxtGrabTarget, public IUxtFarTarget
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	UUxtPinchSliderComponent();

	/** Set collision profile for the slider thumb */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetCollisionProfile(FName Profile);

	/** Set the enabled state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetEnabled(bool bEnabled);

	/** Get the current state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	EUxtSliderState GetCurrentState() const { return CurrentState; }

	/** Get the current grabbed state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	bool IsGrabbed() const  {return CurrentState == EUxtSliderState::Grab;}

	/** Get the current focus state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	bool IsFocused() const { return CurrentState == EUxtSliderState::Focus; }

	/** Get the enabled state of the slider */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	bool IsEnabled() const { return CurrentState != EUxtSliderState::Disabled; }

	/** Get Static Mesh Component used for the thumb visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	UStaticMeshComponent* GetThumbVisuals() const;

	/** Get Static Mesh Component used for the track visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	UStaticMeshComponent* GetTrackVisuals() const;

	/** Get Instanced Static Mesh Component used for the tick marks */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	UInstancedStaticMeshComponent* GetTickMarkVisuals() const;

	/** Set Static Mesh Component used for the thumb visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetThumbVisuals(UStaticMeshComponent* Visuals);

	/** Set Static Mesh Component used for the track visuals */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetTrackVisuals(UStaticMeshComponent* Visuals);

	/** Set Instanced Static Mesh Component used for the tick marks */
	UFUNCTION(BlueprintCallable, Category = "Pinch Slider")
	void SetTickMarkVisuals(UInstancedStaticMeshComponent* Visuals);

	//
	// Getters and setters

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetSliderValue() const { return SliderValue; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetSliderValue(float NewValue);
		
	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	int GetNumTickMarks() const { return NumTickMarks; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetNumTickMarks(int NumTicks);

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetSliderStartDistance() const { return SliderStartDistance; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetSliderStartDistance(float NewStart);

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetSliderEndDistance() const { return SliderEndDistance; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetSliderEndDistance(float NewEnd);

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetSliderLowerBound() const { return SliderLowerBound; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetSliderLowerBound(float NewBound);

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetSliderUpperBound() const { return SliderUpperBound; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetSliderUpperBound(float NewBound);

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	FVector GetTickMarkScale() const { return TickMarkScale; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetTickMarkScale(FVector NewScale);

	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetSmoothing() const { return Smoothing; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetSmoothing(float NewSmoothing);

#if WITH_EDITOR
	/** Editor update function - called by UE4*/
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
	/** Editor update function - called by UE4 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	//
	// Events

	/** Event raised when slider value changes. */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderUpdateValueDelegate OnUpdateValue;

	/** Event raised when slider starts interaction. */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderBeginInteractionDelegate OnBeginInteraction;

	/** Event raised when slider ends interaction. */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderEndInteractionDelegate OnEndInteraction;

	/** Event raised when slider enters focus */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderBeginFocusDelegate OnBeginFocus;

	/** Event raised when slider exits focus */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderEndFocusDelegate OnEndFocus;

	/** Event raised when slider changes state */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderUpdateStateDelegate OnUpdateState;

	/** Event raised when slider changes state */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderEnabledDelegate OnSliderEnabled;

	/** Event raised when slider changes state */
	UPROPERTY(BlueprintAssignable, Category = "Pinch Slider")
	FUxtPinchSliderDisabledDelegate OnSliderDisabled;

protected:

	//
	// UActorComponent interface
	virtual void BeginPlay() override;

	// IUxtGrabTarget interface

	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	//
	// IUxtFarTarget interface

	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarReleased_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarDragged_Implementation(UUxtFarPointerComponent* Pointer) override;

private:

	/** The current value of the slider in 0-1 range */
	UPROPERTY(EditAnywhere, DisplayName = "SliderValue", BlueprintGetter = "GetSliderValue", BlueprintSetter = "SetSliderValue", Category = "Pinch Slider", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SliderValue;

	/** Where the slider track starts, as distance from center along slider axis, in local space units. */
	UPROPERTY(EditAnywhere, DisplayName = "SliderStartDistance", BlueprintGetter = "GetSliderStartDistance", BlueprintSetter = "SetSliderStartDistance", Category = "Pinch Slider")
	float SliderStartDistance;

	/** Where the slider track ends, as distance from center along slider axis, in local space units. */
	UPROPERTY(EditAnywhere, DisplayName = "SliderEndDistance", BlueprintGetter = "GetSliderEndDistance", BlueprintSetter = "SetSliderEndDistance", Category = "Pinch Slider")
	float SliderEndDistance;

	/** The lower bound for the slider value in the range 0-1. */
	UPROPERTY(EditAnywhere, DisplayName = "SliderLowerBound", BlueprintGetter = "GetSliderLowerBound", BlueprintSetter = "SetSliderLowerBound", Category = "Pinch Slider", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SliderLowerBound;

	/** The upper bound for the slider value in the range 0-1. */
	UPROPERTY(EditAnywhere, DisplayName = "SliderUpperBound", BlueprintGetter = "GetSliderUpperBound", BlueprintSetter = "SetSliderUpperBound", Category = "Pinch Slider", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SliderUpperBound;

	/** Number of tick marks to add to the slider */
	UPROPERTY(EditAnywhere, DisplayName = "NumTickMarks", BlueprintGetter = "GetNumTickMarks", BlueprintSetter = "SetNumTickMarks", Category = "Pinch Slider")
	int NumTickMarks;

	/** Scale of the tick mark on the slider */
	UPROPERTY(EditAnywhere, DisplayName = "TickMarkScale", BlueprintGetter = "GetTickMarkScale", BlueprintSetter = "SetTickMarkScale", Category = "Pinch Slider")
	FVector TickMarkScale;

	/** Turns local space position to 0-1 slider scale */
	void UpdateSliderValueFromLocalPosition(float LocalValue);

	/**  Updates thumb position based off 0-1 slider scale */
	void UpdateThumbPositionFromSliderValue();

	/** Use the given mesh to adjust the box component extents. */
	void ConfigureBoxComponent(const UStaticMeshComponent* Mesh);

	/** Internal function to re-initialise component to new state */
	void UpdateSliderState();

	/** Begin focusing the slider */
	void BeginFocus(UUxtPointerComponent* Pointer);

	/** End focusing the slider */
	void EndFocus(UUxtPointerComponent* Pointer);

	/** Begin grabbing the slider */
	void BeginGrab(UUxtPointerComponent* Pointer);

	/** Update the grab on the slider */
	void UpdateGrab(FVector DeltaPos);

	/** End grabbing the slider */
	void EndGrab(UUxtPointerComponent* Pointer);

	/** Visual representation of the slider thumb*/
	UPROPERTY(EditAnywhere, DisplayName = "ThumbVisuals", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"), Category = "Pinch Slider")
	FComponentReference ThumbVisuals;

	/** Visual representation of the track*/
	UPROPERTY(EditAnywhere, DisplayName = "TrackVisuals", meta = (UseComponentPicker, AllowedClasses = "StaticMeshComponent"), Category = "Pinch Slider")
	FComponentReference TrackVisuals;

	/** Visual representation of the tick marks*/
	UPROPERTY(EditAnywhere, DisplayName = "TickMarkVisuals", meta = (UseComponentPicker, AllowedClasses = "InstancedStaticMeshComponent"), Category = "Pinch Slider")
	FComponentReference TickMarkVisuals;

	/** Collision profile used by the slider thumb */
	UPROPERTY(EditAnywhere, Category = "Pinch Slider")
	FName CollisionProfile;

	/** Collision volume used for determining grab events */
	UPROPERTY(Transient)
	class UBoxComponent* BoxComponent;

	/** World space start position for the hand in far grab */
	FVector GrabStartPositionWS;

	/** Local space start position for the thumb in far grab */
	float GrabThumbStartPositionLS;

	/** Current state of the slider */
	EUxtSliderState CurrentState;

	/** Pointer currently grabbing the slider if any */
	TWeakObjectPtr<UUxtPointerComponent> GrabPointerWeak;

	/** Far pointers currently focusing the button */
	TArray<UUxtFarPointerComponent*> FocusingFarPointers;
	
	/** Near pointers currently focusing the button */
	TArray<UUxtNearPointerComponent*> FocusingNearPointers;

	/** 
	 * Motion smoothing factor to apply while manipulating the slider.
	 *
	 * A low-pass filter is applied to the slider's transform to smooth out jittering.
	 * The new transform is an exponentially weighted average of the current transform and the target transform based on the time step:
	 *
	 * T_final = Lerp( T_current, T_target, Exp(-Smoothing * DeltaSeconds) )
	 */
	UPROPERTY(EditAnywhere, DisplayName = "Smoothing", BlueprintGetter = "GetSmoothing", BlueprintSetter = "SetSmoothing", Category = "Pinch Slider", meta = (ClampMin = "0.0"))
	float Smoothing;
};
