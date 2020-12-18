// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/TimelineComponent.h"
#include "Controls/UxtPinchSliderComponent.h"
#include "GameFramework/Actor.h"

#include "UxtPinchSliderActor.generated.h"

class USoundCue;

/**
 * A simple HoloLens 2 style slider that can be moved by grabbing / pinching a slider thumb.
 * The class is extensible to support additional functionality.
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API AUxtPinchSliderActor : public AActor
{
	GENERATED_BODY()

public:
	AUxtPinchSliderActor();

	//
	// Getters and setters.

	// Value.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetValue() const { return Value; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetValue(float NewValue);

	// Minimum value.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetMinValue() const { return MinValue; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetMinValue(float NewMinValue);

	// Maximum value.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetMaxValue() const { return MaxValue; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetMaxValue(float NewMaxValue);

	// Track length.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	float GetTrackLength() const { return TrackLength; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetTrackLength(float NewTrackLength);

	// Step with tick marks.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider")
	bool GetStepWithTickMarks() const { return bStepWithTickMarks; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider")
	void SetStepWithTickMarks(bool bNewStepWithTickMarks);

	// Title.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Text")
	FText GetTitle() const { return Title; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Text")
	void SetTitle(FText NewTitle);

	// Number of decimal places in the value text.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Text")
	int GetValueTextDecimalPlaces() const { return ValueTextDecimalPlaces; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Text")
	void SetValueTextDecimalPlaces(int NewValueTextDecimalPlaces);

	// Align the text with the Z axis.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Text")
	bool GetAlignTextWithZ() const { return bAlignTextWithZ; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Text")
	void SetAlignTextWithZ(bool bNewAlignTextWithZ);

	// Move the text with the thumb.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Text")
	bool GetMoveTextWithThumb() const { return bMoveTextWithThumb; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Text")
	void SetMoveTextWithThumb(bool bNewMoveWithThumb);

	// Number of tick marks.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|TickMarks")
	int GetNumTickMarks() const { return NumTickMarks; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|TickMarks")
	void SetNumTickMarks(int NewNumTickMarks);

	// Tick mark scale.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|TickMarks")
	FVector GetTickMarkScale() const { return TickMarkScale; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|TickMarks")
	void SetTickMarkScale(FVector NewTickMarkScale);

	// Default color.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Visuals")
	FLinearColor GetDefaultThumbColor() const { return DefaultThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Visuals")
	void SetDefaultThumbColor(FLinearColor NewDefaultThumbColor);

	// Focused color.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Visuals")
	FLinearColor GetFocusedThumbColor() const { return FocusedThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Visuals")
	void SetFocusedThumbColor(FLinearColor NewFocusedThumbColor);

	// Grabbed color.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Visuals")
	FLinearColor GetGrabbedThumbColor() const { return GrabbedThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Visuals")
	void SetGrabbedThumbColor(FLinearColor NewGrabbedThumbColor);

	// Disabled color.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Visuals")
	FLinearColor GetDisabledThumbColor() const { return DisabledThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Visuals")
	void SetDisabledThumbColor(FLinearColor NewDisabledThumbColor);

	// Grab sound.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Audio")
	USoundCue* GetGrabSound() const { return GrabSound; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Audio")
	void SetGrabSound(USoundCue* NewGrabSound);

	// Release sound.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Audio")
	USoundCue* GetReleaseSound() const { return ReleaseSound; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Audio")
	void SetReleaseSound(USoundCue* NewReleaseSound);

	// Tick sound.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Audio")
	USoundCue* GetTickSound() const { return TickSound; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Audio")
	void SetTickSound(USoundCue* NewTickSound);

	// Default thumb scale.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Thumb")
	float GetDefaultThumbScale() const { return DefaultThumbScale; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Thumb")
	void SetDefaultThumbScale(float NewDefaultThumbScale);

	// Focused thumb scale.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Thumb")
	float GetFocusedThumbScale() const { return FocusedThumbScale; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Thumb")
	void SetFocusedThumbScale(float NewFocusedThumbScale);

	// Thumb scale curve.
	UFUNCTION(BlueprintGetter, Category = "Uxt Pinch Slider|Thumb")
	UCurveFloat* GetThumbScaleCurve() const { return ThumbScaleCurve; }
	UFUNCTION(BlueprintSetter, Category = "Uxt Pinch Slider|Thumb")
	void SetThumbScaleCurve(UCurveFloat* NewThumbScaleCurve);

	//
	// Events

	/** Event raised when the slider's value changes. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Pinch Slider")
	void OnSliderUpdateValue(float NewValue);

protected:
	//
	// AActor interface.

	virtual void OnConstruction(const FTransform& Transform) override;

	//
	// PinchSlider event callbacks.

	UFUNCTION(Category = "Uxt Pinch Slider")
	virtual void OnUpdateState(UUxtPinchSliderComponent* Slider, EUxtSliderState NewState);

	UFUNCTION(Category = "Uxt Pinch Slider")
	virtual void OnBeginGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer);

	UFUNCTION(Category = "Uxt Pinch Slider")
	virtual void OnUpdateValue(UUxtPinchSliderComponent* Slider, float NewValue);

	UFUNCTION(Category = "Uxt Pinch Slider")
	virtual void OnEndGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer);

	//
	// ScaleTimeline event callbacks.

	UFUNCTION(Category = "Uxt Pinch Slider")
	void OnUpdateTimeline(float Scale);

	//
	// Components.

	/** The slider functionality. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Visuals")
	UUxtPinchSliderComponent* PinchSlider;

	/** The thumb visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Visuals")
	UStaticMeshComponent* Thumb;

	/** The track visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Visuals")
	UStaticMeshComponent* Track;

	/** The tick mark visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Visuals")
	class UInstancedStaticMeshComponent* TickMarks;

	/** Root text component to allow text to move as a block. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Text")
	USceneComponent* TextRoot;

	/** The title text, configure using the 'Title' property. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Text")
	class UTextRenderComponent* TitleText;

	/** The value text. TODO configure */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Text")
	class UTextRenderComponent* ValueText;

	/** The audio cues, configure using the 'Slider Sounds' properties. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider|Audio")
	class UAudioComponent* Audio;

	/** The timeline for scaling the thumb, configuring using the 'Slider Thumb Scaling' properties. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pinch Slider")
	UTimelineComponent* ScaleTimeline;

private:
	//
	// Private interface.

	float ToNormalizedValue(float RawValue) const;
	float FromNormalizedValue(float NormalizedValue) const;

	//
	// Internal state updates.

	void UpdateTrack();
	void UpdateTickMarks();
	void UpdateText();

	//
	// Configurable properties.

	/** The slider's initial value. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetValue, BlueprintSetter = SetValue)
	float Value = 0.0f;

	/** The slider's minimum value. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetMinValue, BlueprintSetter = SetMinValue)
	float MinValue = 0.0f;

	/** The slider's maximum value. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetMaxValue, BlueprintSetter = SetMaxValue)
	float MaxValue = 1.0f;

	/** The length of the slider's track. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetTrackLength, BlueprintSetter = SetTrackLength,
		meta = (ClampMin = 0.0f))
	float TrackLength = 50.0f;

	/** Use stepped movement and step with the tick marks. Requires at least two tick marks. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider", BlueprintGetter = GetStepWithTickMarks, BlueprintSetter = SetStepWithTickMarks,
		meta = (EditCondition = "NumTickMarks >= 2"))
	bool bStepWithTickMarks = false;

	/** The title text. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|Text", BlueprintGetter = GetTitle, BlueprintSetter = SetTitle)
	FText Title = NSLOCTEXT("UxtPinchSliderActor", "TitleDefault", "Title");

	/** The number of decimal places to show in the value text. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Text", BlueprintGetter = GetValueTextDecimalPlaces,
		BlueprintSetter = SetValueTextDecimalPlaces, meta = (ClampMin = 0))
	int ValueTextDecimalPlaces = 2;

	/** Align the text with the Z axis. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|Text", BlueprintGetter = GetAlignTextWithZ, BlueprintSetter = SetAlignTextWithZ)
	bool bAlignTextWithZ = true;

	/** Move the text with the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Text", BlueprintGetter = GetMoveTextWithThumb, BlueprintSetter = SetMoveTextWithThumb)
	bool bMoveTextWithThumb = true;

	/** The number of tick marks along the track. Must be at least two if using Step With Tick Marks. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|TickMarks", BlueprintGetter = GetNumTickMarks, BlueprintSetter = SetNumTickMarks,
		meta = (ClampMin = 0))
	int NumTickMarks = 5;

	/** The scale of the tick marks. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|TickMarks", BlueprintGetter = GetTickMarkScale, BlueprintSetter = SetTickMarkScale)
	FVector TickMarkScale = FVector(0.0075f);

	/** The default color for the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Visuals", BlueprintGetter = GetDefaultThumbColor, BlueprintSetter = SetDefaultThumbColor)
	FLinearColor DefaultThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("E1E1E1FF"));

	/** The focused color for the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Visuals", BlueprintGetter = GetFocusedThumbColor, BlueprintSetter = SetFocusedThumbColor)
	FLinearColor FocusedThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("0082D8FF"));

	/** The grabbed color for the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Visuals", BlueprintGetter = GetGrabbedThumbColor, BlueprintSetter = SetGrabbedThumbColor)
	FLinearColor GrabbedThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("15DFDAFF"));

	/** The disabled color for the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Visuals", BlueprintGetter = GetDisabledThumbColor,
		BlueprintSetter = SetDisabledThumbColor)
	FLinearColor DisabledThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("898989FF"));

	/** The sound played when the slider is grabbed. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|Audio", BlueprintGetter = GetGrabSound, BlueprintSetter = SetGrabSound)
	USoundCue* GrabSound;

	/** The sound played when the slider is released. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|Audio", BlueprintGetter = GetReleaseSound, BlueprintSetter = SetReleaseSound)
	USoundCue* ReleaseSound;

	/** The sound played when the slider passes a tick mark. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|Audio", BlueprintGetter = GetTickSound, BlueprintSetter = SetTickSound)
	USoundCue* TickSound;

	/** The default scale for the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Thumb", BlueprintGetter = GetDefaultThumbScale, BlueprintSetter = SetDefaultThumbScale)
	float DefaultThumbScale = 0.75f;

	/** The focused scale of the thumb. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Pinch Slider|Thumb", BlueprintGetter = GetFocusedThumbScale, BlueprintSetter = SetFocusedThumbScale)
	float FocusedThumbScale = 1.0f;

	/** The focused scale of the thumb. */
	UPROPERTY(EditAnywhere, Category = "Uxt Pinch Slider|Thumb", BlueprintGetter = GetThumbScaleCurve, BlueprintSetter = SetThumbScaleCurve)
	UCurveFloat* ThumbScaleCurve;

	//
	// Private properties.

	/** The scale timeline callback. */
	FOnTimelineFloat ScaleTimelineCallback;
};
