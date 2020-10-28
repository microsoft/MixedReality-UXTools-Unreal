// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPinchSliderActor.h"

#include "UXTools.h"

#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundCue.h"
#include "UObject/ConstructorHelpers.h"

AUxtPinchSliderActor::AUxtPinchSliderActor()
{
	// Default assets.

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(
		TEXT("Material'/UXTools/Materials/M_SimpleLit_Color.M_SimpleLit_Color'"));
	check(DefaultMaterial.Object);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultThumbMesh(
		TEXT("StaticMesh'/UXTools/Slider/Meshes/SM_Button_Oval_Concave_12x24mm_optimized.SM_Button_Oval_Concave_12x24mm_optimized'"));
	check(DefaultThumbMesh.Object);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultTrackMesh(
		TEXT("StaticMesh'/UXTools/Slider/Meshes/SM_Slider_Track_Simple.SM_Slider_Track_Simple'"));
	check(DefaultTrackMesh.Object);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultTickMarkMesh(TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'"));
	check(DefaultTickMarkMesh.Object);

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultTextMaterial(TEXT("Material'/UXTools/Fonts/M_DefaultFont.M_DefaultFont'"));
	check(DefaultTextMaterial.Object);

	static ConstructorHelpers::FObjectFinder<UFont> DefaultTextFont(
		TEXT("Font'/UXTools/Fonts/Font_SegoeUI_Semibold_42.Font_SegoeUI_Semibold_42'"));
	check(DefaultTextFont.Object);

	static ConstructorHelpers::FObjectFinder<USoundCue> DefaultGrabSound(
		TEXT("SoundCue'/UXTools/Slider/Audio/A_Slider_Grab_Cue.A_Slider_Grab_Cue'"));
	check(DefaultGrabSound.Object);

	static ConstructorHelpers::FObjectFinder<USoundCue> DefaultReleaseSound(
		TEXT("SoundCue'/UXTools/Slider/Audio/A_Slider_Release_Cue.A_Slider_Release_Cue'"));
	check(DefaultReleaseSound.Object);

	static ConstructorHelpers::FObjectFinder<USoundCue> DefaultTickSound(
		TEXT("SoundCue'/UXTools/Slider/Audio/A_Slider_Pass_Notch_Cue.A_Slider_Pass_Notch_Cue'"));
	check(DefaultTickSound.Object);

	static ConstructorHelpers::FObjectFinder<UCurveFloat> DefaultThumbScaleCurve(
		TEXT("CurveFloat'/UXTools/Slider/Anim/Curve_SliderButtonScale_Float.Curve_SliderButtonScale_Float'"));
	check(DefaultThumbScaleCurve.Object);

	// Components.

	PinchSlider = CreateDefaultSubobject<UUxtPinchSliderComponent>("PinchSlider");
	PinchSlider->OnUpdateState.AddDynamic(this, &AUxtPinchSliderActor::OnUpdateState);
	PinchSlider->OnBeginGrab.AddDynamic(this, &AUxtPinchSliderActor::OnBeginGrab);
	PinchSlider->OnUpdateValue.AddDynamic(this, &AUxtPinchSliderActor::OnUpdateValue);
	PinchSlider->OnEndGrab.AddDynamic(this, &AUxtPinchSliderActor::OnEndGrab);
	SetRootComponent(PinchSlider);

	Thumb = CreateDefaultSubobject<UStaticMeshComponent>("Thumb");
	Thumb->SetupAttachment(PinchSlider);
	Thumb->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, 180.0f)));
	Thumb->SetStaticMesh(DefaultThumbMesh.Object);
	Thumb->SetMaterial(0, DefaultMaterial.Object);

	FComponentReference ThumbReference;
	ThumbReference.PathToComponent = Thumb->GetName();
	PinchSlider->SetVisuals(ThumbReference);

	Track = CreateDefaultSubobject<UStaticMeshComponent>("Track");
	Track->SetupAttachment(PinchSlider);
	Track->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, 180.0f)));
	Track->SetStaticMesh(DefaultTrackMesh.Object);
	Track->SetMaterial(0, DefaultMaterial.Object);

	TickMarks = CreateDefaultSubobject<UInstancedStaticMeshComponent>("TickMarks");
	TickMarks->SetupAttachment(PinchSlider);
	TickMarks->SetRelativeLocation(FVector(0.0f, 0.0f, -1.0f));
	TickMarks->SetRelativeRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, 180.0f)));
	TickMarks->SetStaticMesh(DefaultTickMarkMesh.Object);
	TickMarks->SetMaterial(0, DefaultMaterial.Object);

	TextRoot = CreateDefaultSubobject<USceneComponent>("TextRoot");
	TextRoot->SetupAttachment(PinchSlider);
	TextRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 3.5f));

	TitleText = CreateDefaultSubobject<UTextRenderComponent>("TitleText");
	TitleText->SetupAttachment(TextRoot);
	TitleText->SetRelativeLocation(FVector(0.0f, 0.0f, 1.0f));
	TitleText->SetTextMaterial(DefaultTextMaterial.Object);
	TitleText->SetFont(DefaultTextFont.Object);
	TitleText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TitleText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	TitleText->SetWorldSize(2);

	ValueText = CreateDefaultSubobject<UTextRenderComponent>("ValueText");
	ValueText->SetupAttachment(TextRoot);
	ValueText->SetRelativeLocation(FVector(0.0f, 0.0f, -1.0f));
	ValueText->SetTextMaterial(DefaultTextMaterial.Object);
	ValueText->SetFont(DefaultTextFont.Object);
	ValueText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	ValueText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	ValueText->SetWorldSize(3);

	Audio = CreateDefaultSubobject<UAudioComponent>("Audio");
	Audio->SetupAttachment(Thumb);
	Audio->SetAutoActivate(false);

	ScaleTimeline = CreateDefaultSubobject<UTimelineComponent>("ScaleTimeline");
	ScaleTimeline->SetPlayRate(3.5f);

	// Properties.

	GrabSound = DefaultGrabSound.Object;
	ReleaseSound = DefaultReleaseSound.Object;
	TickSound = DefaultTickSound.Object;
	ThumbScaleCurve = DefaultThumbScaleCurve.Object;

	ScaleTimelineCallback.BindDynamic(this, &AUxtPinchSliderActor::OnUpdateTimeline);
}

void AUxtPinchSliderActor::SetValue(float NewValue)
{
	Value = FMath::Clamp(NewValue, MinValue, MaxValue);
	PinchSlider->SetValue(ToNormalizedValue(Value));

	UpdateText();
}

void AUxtPinchSliderActor::SetMinValue(float NewMinValue)
{
	MinValue = NewMinValue;
}

void AUxtPinchSliderActor::SetMaxValue(float NewMaxValue)
{
	MaxValue = NewMaxValue;
}

void AUxtPinchSliderActor::SetTrackLength(float NewTrackLength)
{
	TrackLength = FMath::Max(0.0f, NewTrackLength);
	PinchSlider->SetTrackLength(TrackLength);
	UpdateTrack();
}

void AUxtPinchSliderActor::SetStepWithTickMarks(bool bNewStepWithTickMarks)
{
	bStepWithTickMarks = bNewStepWithTickMarks;
	PinchSlider->SetUseSteppedMovement(bStepWithTickMarks);
	UpdateTickMarks();
}

void AUxtPinchSliderActor::SetTitle(FText NewTitle)
{
	Title = NewTitle;
	TitleText->SetText(NewTitle);
}

void AUxtPinchSliderActor::SetValueTextDecimalPlaces(int NewValueTextDecimalPlaces)
{
	ValueTextDecimalPlaces = FMath::Max(0, NewValueTextDecimalPlaces);
	UpdateText();
}

void AUxtPinchSliderActor::SetAlignTextWithZ(bool bNewAlignTextWithZ)
{
	bAlignTextWithZ = bNewAlignTextWithZ;
	UpdateText();
}

void AUxtPinchSliderActor::SetMoveTextWithThumb(bool bNewMoveWithThumb)
{
	bMoveTextWithThumb = bNewMoveWithThumb;
	UpdateText();
}

void AUxtPinchSliderActor::SetNumTickMarks(int NewNumTickMarks)
{
	NumTickMarks = FMath::Max(0, NewNumTickMarks);
	UpdateTickMarks();
}

void AUxtPinchSliderActor::SetTickMarkScale(FVector NewTickMarkScale)
{
	TickMarkScale = NewTickMarkScale;
	UpdateTickMarks();
}

void AUxtPinchSliderActor::SetDefaultThumbColor(FLinearColor NewDefaultThumbColor)
{
	DefaultThumbColor = NewDefaultThumbColor;
	OnUpdateState(PinchSlider, PinchSlider->GetState());
}

void AUxtPinchSliderActor::SetFocusedThumbColor(FLinearColor NewFocusedThumbColor)
{
	FocusedThumbColor = NewFocusedThumbColor;
	OnUpdateState(PinchSlider, PinchSlider->GetState());
}

void AUxtPinchSliderActor::SetGrabbedThumbColor(FLinearColor NewGrabbedThumbColor)
{
	GrabbedThumbColor = NewGrabbedThumbColor;
	OnUpdateState(PinchSlider, PinchSlider->GetState());
}

void AUxtPinchSliderActor::SetDisabledThumbColor(FLinearColor NewDisabledThumbColor)
{
	DisabledThumbColor = NewDisabledThumbColor;
	OnUpdateState(PinchSlider, PinchSlider->GetState());
}

void AUxtPinchSliderActor::SetGrabSound(USoundCue* NewGrabSound)
{
	GrabSound = NewGrabSound;
}

void AUxtPinchSliderActor::SetReleaseSound(USoundCue* NewReleaseSound)
{
	ReleaseSound = NewReleaseSound;
}

void AUxtPinchSliderActor::SetTickSound(USoundCue* NewTickSound)
{
	TickSound = NewTickSound;
}

void AUxtPinchSliderActor::SetDefaultThumbScale(float NewDefaultThumbScale)
{
	DefaultThumbScale = NewDefaultThumbScale;
	OnUpdateState(PinchSlider, PinchSlider->GetState());
}

void AUxtPinchSliderActor::SetFocusedThumbScale(float NewFocusedThumbScale)
{
	FocusedThumbScale = NewFocusedThumbScale;
	OnUpdateState(PinchSlider, PinchSlider->GetState());
}

void AUxtPinchSliderActor::SetThumbScaleCurve(UCurveFloat* NewThumbScaleCurve)
{
	ThumbScaleCurve = NewThumbScaleCurve;
	ScaleTimeline->AddInterpFloat(ThumbScaleCurve, ScaleTimelineCallback, "Scale");
}

void AUxtPinchSliderActor::OnSliderUpdateValue_Implementation(float NewValue)
{
}

void AUxtPinchSliderActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	PinchSlider->SetValue(ToNormalizedValue(Value));
	PinchSlider->SetTrackLength(TrackLength);
	PinchSlider->SetUseSteppedMovement(bStepWithTickMarks);

	Thumb->SetRelativeScale3D(FVector(DefaultThumbScale));
	Thumb->SetVectorParameterValueOnMaterials("Base Color", FVector(DefaultThumbColor.R, DefaultThumbColor.G, DefaultThumbColor.B));

	TitleText->SetText(Title);

	ScaleTimeline->AddInterpFloat(ThumbScaleCurve, ScaleTimelineCallback, "Scale");

	UpdateTrack();
	UpdateTickMarks();
	UpdateText();
}

void AUxtPinchSliderActor::OnUpdateState(UUxtPinchSliderComponent* Slider, EUxtSliderState NewState)
{
	switch (NewState)
	{
	case EUxtSliderState::Default:
		Thumb->SetVectorParameterValueOnMaterials("Base Color", FVector(DefaultThumbColor.R, DefaultThumbColor.G, DefaultThumbColor.B));
		ScaleTimeline->Reverse();
		break;

	case EUxtSliderState::Focused:
		Thumb->SetVectorParameterValueOnMaterials("Base Color", FVector(FocusedThumbColor.R, FocusedThumbColor.G, FocusedThumbColor.B));
		ScaleTimeline->Play();
		break;

	case EUxtSliderState::Grabbed:
		Thumb->SetVectorParameterValueOnMaterials("Base Color", FVector(GrabbedThumbColor.R, GrabbedThumbColor.G, GrabbedThumbColor.B));
		ScaleTimeline->Reverse();
		break;

	case EUxtSliderState::Disabled:
		Thumb->SetVectorParameterValueOnMaterials("Base Color", FVector(DisabledThumbColor.R, DisabledThumbColor.G, DisabledThumbColor.B));
		ScaleTimeline->Reverse();
		break;

	default:
		checkNoEntry();
	}
}

void AUxtPinchSliderActor::OnBeginGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer)
{
	Audio->SetSound(GrabSound);
	Audio->SetFloatParameter("Pitch", 0.5f);
	Audio->Play();
}

void AUxtPinchSliderActor::OnUpdateValue(UUxtPinchSliderComponent* Slider, float NewValue)
{
	if (!FMath::IsNearlyEqual(Value, FromNormalizedValue(NewValue)))
	{
		// Tick audio.
		const float PreviousValue = ToNormalizedValue(Value);
		const float NumZones = NumTickMarks - 1;
		const float PreviousZone = PreviousValue * NumZones;
		const float PreviousZoneCeil = FMath::CeilToFloat(PreviousZone) / NumZones;
		const float PreviousZoneFloor = FMath::FloorToFloat(PreviousZone) / NumZones;

		if (NewValue <= PreviousZoneFloor || NewValue >= PreviousZoneCeil)
		{
			Audio->SetSound(TickSound);
			Audio->SetFloatParameter("Pitch", NewValue);
			Audio->Play();
		}

		// Update value.
		Value = FromNormalizedValue(NewValue);
		UpdateText();
		OnSliderUpdateValue(Value);
	}
}

void AUxtPinchSliderActor::OnEndGrab(UUxtPinchSliderComponent* Slider, UUxtPointerComponent* Pointer)
{
	Audio->SetSound(ReleaseSound);
	Audio->SetFloatParameter("Pitch", 0.5f);
	Audio->Play();
}

void AUxtPinchSliderActor::OnUpdateTimeline(float Scale)
{
	Thumb->SetRelativeScale3D(FVector(((FocusedThumbScale - DefaultThumbScale) * Scale) + DefaultThumbScale));
}

float AUxtPinchSliderActor::ToNormalizedValue(float RawValue) const
{
	return (RawValue - MinValue) / (MaxValue - MinValue);
}

float AUxtPinchSliderActor::FromNormalizedValue(float NormalizedValue) const
{
	return (NormalizedValue * (MaxValue - MinValue)) + MinValue;
}

void AUxtPinchSliderActor::UpdateTrack()
{
	FVector TrackMin, TrackMax;
	Track->GetLocalBounds(TrackMin, TrackMax);

	const FVector TrackScale = Track->GetRelativeScale3D();
	Track->SetRelativeScale3D(FVector(TrackScale.X, TrackLength / (TrackMax.Y - TrackMin.Y), TrackScale.Z));
}

void AUxtPinchSliderActor::UpdateTickMarks()
{
	// Set the number of steps to the number of tick marks if using stepped movement.
	if (bStepWithTickMarks)
	{
		// If using stepped movement, there must be at least two tick marks.
		if (NumTickMarks < 2)
		{
			UE_LOG(UXTools, Warning, TEXT("Attempted to use stepped slider movement with less than two tick marks."));
			NumTickMarks = 2;
		}

		PinchSlider->SetNumSteps(NumTickMarks);
	}

	// Update meshes.
	TickMarks->ClearInstances();
	if (NumTickMarks == 1)
	{
		const FTransform Transform(FQuat::Identity, FVector::ZeroVector, TickMarkScale);
		TickMarks->AddInstance(Transform);
	}
	else if (NumTickMarks > 1)
	{
		const float Step = TrackLength / (NumTickMarks - 1);
		FTransform Transform(FQuat::Identity, FVector(0.0f, -(TrackLength / 2.0f), 0.0f), TickMarkScale);

		for (int i = 0; i < NumTickMarks; ++i)
		{
			TickMarks->AddInstance(Transform);
			Transform.AddToTranslation(FVector(0.0f, Step, 0.0f));
		}
	}
}

void AUxtPinchSliderActor::UpdateText()
{
	// Update value text.
	FNumberFormattingOptions NumberFormat;
	NumberFormat.MinimumFractionalDigits = ValueTextDecimalPlaces;
	NumberFormat.MaximumFractionalDigits = ValueTextDecimalPlaces;
	ValueText->SetText(FText::AsNumber(Value, &NumberFormat));

	// Align the text with the Z axis.
	if (bAlignTextWithZ)
	{
		const FVector TextRotation = TextRoot->GetComponentRotation().Euler();
		TextRoot->SetWorldRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, TextRotation.Z)));
	}

	// Move the text with the thumb.
	if (bMoveTextWithThumb)
	{
		const FVector ThumbLocation = Thumb->GetRelativeLocation();
		const FVector TextRootLocation = TextRoot->GetRelativeLocation();
		TextRoot->SetRelativeLocation(FVector(TextRootLocation.X, ThumbLocation.Y, TextRootLocation.Z));
	}
}
