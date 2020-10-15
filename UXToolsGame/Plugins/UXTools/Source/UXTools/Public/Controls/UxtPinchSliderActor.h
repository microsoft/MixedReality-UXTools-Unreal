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
UCLASS(ClassGroup = UXTools)
class UXTOOLS_API AUxtPinchSliderActor : public AActor
{
	GENERATED_BODY()

public:
	AUxtPinchSliderActor();

	//
	// Getters and setters.

	// Initial value.
	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetInitialValue() const { return InitialValue; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetInitialValue(float NewInitialValue);

	// Track length.
	UFUNCTION(BlueprintGetter, Category = "Pinch Slider")
	float GetTrackLength() const { return TrackLength; }
	UFUNCTION(BlueprintSetter, Category = "Pinch Slider")
	void SetTrackLength(float NewTrackLength);

	// Title.
	UFUNCTION(BlueprintGetter, Category = "Slider Text")
	FText GetTitle() const { return Title; }
	UFUNCTION(BlueprintSetter, Category = "Slider Text")
	void SetTitle(FText NewTitle);

	// Number of decimal places in the value text.
	UFUNCTION(BlueprintGetter, Category = "Slider Text")
	int GetValueTextDecimalPlaces() const { return ValueTextDecimalPlaces; }
	UFUNCTION(BlueprintSetter, Category = "Slider Text")
	void SetValueTextDecimalPlaces(int NewValueTextDecimalPlaces);

	// Align the text with the Z axis.
	UFUNCTION(BlueprintGetter, Category = "Slider Text")
	bool GetAlignTextWithZ() const { return bAlignTextWithZ; }
	UFUNCTION(BlueprintSetter, Category = "Slider Text")
	void SetAlignTextWithZ(bool bNewAlignTextWithZ);

	// Move the text with the thumb.
	UFUNCTION(BlueprintGetter, Category = "Slider Text")
	bool GetMoveTextWithThumb() const { return bMoveTextWithThumb; }
	UFUNCTION(BlueprintSetter, Category = "Slider Text")
	void SetMoveTextWithThumb(bool bNewMoveWithThumb);

	// Number of tick marks.
	UFUNCTION(BlueprintGetter, Category = "Slider Tick Marks")
	int GetNumTickMarks() const { return NumTickMarks; }
	UFUNCTION(BlueprintSetter, Category = "Slider Tick Marks")
	void SetNumTickMarks(int NewNumTickMarks);

	// Tick mark scale.
	UFUNCTION(BlueprintGetter, Category = "Slider Tick Marks")
	FVector GetTickMarkScale() const { return TickMarkScale; }
	UFUNCTION(BlueprintSetter, Category = "Slider Tick Marks")
	void SetTickMarkScale(FVector NewTickMarkScale);

	// Default color.
	UFUNCTION(BlueprintGetter, Category = "Slider Colors")
	FLinearColor GetDefaultThumbColor() const { return DefaultThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Slider Colors")
	void SetDefaultThumbColor(FLinearColor NewDefaultThumbColor);

	// Focused color.
	UFUNCTION(BlueprintGetter, Category = "Slider Colors")
	FLinearColor GetFocusedThumbColor() const { return FocusedThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Slider Colors")
	void SetFocusedThumbColor(FLinearColor NewFocusedThumbColor);

	// Grabbed color.
	UFUNCTION(BlueprintGetter, Category = "Slider Colors")
	FLinearColor GetGrabbedThumbColor() const { return GrabbedThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Slider Colors")
	void SetGrabbedThumbColor(FLinearColor NewGrabbedThumbColor);

	// Disabled color.
	UFUNCTION(BlueprintGetter, Category = "Slider Colors")
	FLinearColor GetDisabledThumbColor() const { return DisabledThumbColor; }
	UFUNCTION(BlueprintSetter, Category = "Slider Colors")
	void SetDisabledThumbColor(FLinearColor NewDisabledThumbColor);

	// Grab sound.
	UFUNCTION(BlueprintGetter, Category = "Slider Sounds")
	USoundCue* GetGrabSound() const { return GrabSound; }
	UFUNCTION(BlueprintSetter, Category = "Slider Sounds")
	void SetGrabSound(USoundCue* NewGrabSound);

	// Release sound.
	UFUNCTION(BlueprintGetter, Category = "Slider Sounds")
	USoundCue* GetReleaseSound() const { return ReleaseSound; }
	UFUNCTION(BlueprintSetter, Category = "Slider Sounds")
	void SetReleaseSound(USoundCue* NewReleaseSound);

	// Tick sound.
	UFUNCTION(BlueprintGetter, Category = "Slider Sounds")
	USoundCue* GetTickSound() const { return TickSound; }
	UFUNCTION(BlueprintSetter, Category = "Slider Sounds")
	void SetTickSound(USoundCue* NewTickSound);

	// Default thumb scale.
	UFUNCTION(BlueprintGetter, Category = "Slider Thumb Scaling")
	float GetDefaultThumbScale() const { return DefaultThumbScale; }
	UFUNCTION(BlueprintSetter, Category = "Slider Thumb Scaling")
	void SetDefaultThumbScale(float NewDefaultThumbScale);

	// Focused thumb scale.
	UFUNCTION(BlueprintGetter, Category = "Slider Thumb Scaling")
	float GetFocusedThumbScale() const { return FocusedThumbScale; }
	UFUNCTION(BlueprintSetter, Category = "Slider Thumb Scaling")
	void SetFocusedThumbScale(float NewFocusedThumbScale);

	// Thumb scale curve.
	UFUNCTION(BlueprintGetter, Category = "Slider Thumb Scaling")
	UCurveFloat* GetThumbScaleCurve() const { return ThumbScaleCurve; }
	UFUNCTION(BlueprintSetter, Category = "Slider Thumb Scaling")
	void SetThumbScaleCurve(UCurveFloat* NewThumbScaleCurve);

protected:
	//
	// AActor interface.

	virtual void OnConstruction(const FTransform& Transform) override;

	//
	// PinchSlider event callbacks.

	UFUNCTION()
	virtual void OnUpdateState(UUxtPinchSliderComponent* Slider, EUxtSliderState NewState);

	UFUNCTION()
	virtual void OnBeginGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer);

	UFUNCTION()
	virtual void OnUpdateValue(UUxtPinchSliderComponent* Slider, float NewValue);

	UFUNCTION()
	virtual void OnEndGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer);

	//
	// ScaleTimeline event callbacks.

	UFUNCTION()
	void OnUpdateTimeline(float Scale);

	//
	// Components.

	/** The slider functionality. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	UUxtPinchSliderComponent* PinchSlider;

	/** The thumb visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	UStaticMeshComponent* Thumb;

	/** The track visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	UStaticMeshComponent* Track;

	/** The tick mark visuals. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	class UInstancedStaticMeshComponent* TickMarks;

	/** Root text component to allow text to move as a block. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	USceneComponent* TextRoot;

	/** The title text, configure using the 'Title' property. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	class UTextRenderComponent* TitleText;

	/** The value text. TODO configure */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	class UTextRenderComponent* ValueText;

	/** The audio cues, configure using the 'Slider Sounds' properties. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	class UAudioComponent* Audio;

	/** The timeline for scaling the thumb, configuring using the 'Slider Thumb Scaling' properties. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Slider Components")
	UTimelineComponent* ScaleTimeline;

private:
	//
	// Internal state updates.

	void UpdateVisuals();
	void UpdateText(float Value);

	//
	// Configurable properties.

	/** The slider's initial value, between 0-1. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetInitialValue, BlueprintSetter = SetInitialValue, Category = "Pinch Slider",
		meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float InitialValue = 0.0f;

	/** The length of the slider's track. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetTrackLength, BlueprintSetter = SetTrackLength, Category = "Pinch Slider",
		meta = (ClampMin = 0.0f))
	float TrackLength = 50.0f;

	/** The title text. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetTitle, BlueprintSetter = SetTitle, Category = "Slider Text")
	FText Title = NSLOCTEXT("UxtPinchSliderActor", "TitleDefault", "Title");

	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetValueTextDecimalPlaces, BlueprintSetter = SetValueTextDecimalPlaces, Category = "Slider Text",
		meta = (ClampMin = 0))
	int ValueTextDecimalPlaces = 2;

	/** Align the text with the Z axis. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetAlignTextWithZ, BlueprintSetter = SetAlignTextWithZ, Category = "Slider Text")
	bool bAlignTextWithZ = true;

	/** Move the text with the thumb. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetMoveTextWithThumb, BlueprintSetter = SetMoveTextWithThumb, Category = "Slider Text")
	bool bMoveTextWithThumb = true;

	/** The number of tick marks along the track. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetNumTickMarks, BlueprintSetter = SetNumTickMarks, Category = "Slider Tick Marks",
		meta = (ClampMin = 0))
	int NumTickMarks = 5;

	/** The scale of the tick marks. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetTickMarkScale, BlueprintSetter = SetTickMarkScale, Category = "Slider Tick Marks")
	FVector TickMarkScale = FVector(0.0075f);

	/** The default color for the thumb. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetDefaultThumbColor, BlueprintSetter = SetDefaultThumbColor, Category = "Slider Colors")
	FLinearColor DefaultThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("E1E1E1FF"));

	/** The focused color for the thumb. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetFocusedThumbColor, BlueprintSetter = SetFocusedThumbColor, Category = "Slider Colors")
	FLinearColor FocusedThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("0082D8FF"));

	/** The grabbed color for the thumb. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetGrabbedThumbColor, BlueprintSetter = SetGrabbedThumbColor, Category = "Slider Colors")
	FLinearColor GrabbedThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("15DFDAFF"));

	/** The disabled color for the thumb. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetDisabledThumbColor, BlueprintSetter = SetDisabledThumbColor, Category = "Slider Colors")
	FLinearColor DisabledThumbColor = FLinearColor::FromSRGBColor(FColor::FromHex("898989FF"));

	/** The sound played when the slider is grabbed. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetGrabSound, BlueprintSetter = SetGrabSound, Category = "Slider Sounds")
	USoundCue* GrabSound;

	/** The sound played when the slider is released. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetReleaseSound, BlueprintSetter = SetReleaseSound, Category = "Slider Sounds")
	USoundCue* ReleaseSound;

	/** The sound played when the slider passes a tick mark. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetTickSound, BlueprintSetter = SetTickSound, Category = "Slider Sounds")
	USoundCue* TickSound;

	/** The default scale for the thumb. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetDefaultThumbScale, BlueprintSetter = SetDefaultThumbScale, Category = "Slider Thumb Scaling")
	float DefaultThumbScale = 0.75f;

	/** The focused scale of the thumb. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetFocusedThumbScale, BlueprintSetter = SetFocusedThumbScale, Category = "Slider Thumb Scaling")
	float FocusedThumbScale = 1.0f;

	/** The focused scale of the thumb. */
	UPROPERTY(EditAnywhere, BlueprintGetter = GetThumbScaleCurve, BlueprintSetter = SetThumbScaleCurve, Category = "Slider Thumb Scaling")
	UCurveFloat* ThumbScaleCurve;

	//
	// Private properties.

	/** The slider's previous value */
	float PreviousValue;

	/** The scale timeline callback. */
	FOnTimelineFloat ScaleTimelineCallback;
};
