// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UxtButtonBrush.generated.h"

class UMaterialInterface;
class UCurveFloat;
class UStaticMesh;
class USoundBase;

/**
 * Structure containing data representing button visual assets and properties.
 */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtButtonVisualsBrush
{
	GENERATED_BODY()

public:
	/** The material used for the button back plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UMaterialInterface* BackPlateMaterial = nullptr;

	/** The mesh used for the button back plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UStaticMesh* BackPlateMesh = nullptr;

	/** The material used for the button front plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UMaterialInterface* FrontPlateMaterial = nullptr;

	/** The mesh used for the button front plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UStaticMesh* FrontPlateMesh = nullptr;

	/** Handle to the default left pulse materials to use for the button front plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UMaterialInterface* FrontPlatePulseLeftMaterial;

	/** Handle to the default right pulse materials to use for the button front plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UMaterialInterface* FrontPlatePulseRightMaterial;

	/** How long it takes the front plate pulse to animate in size in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	float PulseTime = 0.4f;

	/** How long it takes the front plate pulse to fade out in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	float PulseFadeTime = 0.125f;

	/** When a button is focused, how quickly the icon animates to the focused location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	float IconFocusSpeed = 20.f;

	/** Curve which describes the motion of the focus animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UCurveFloat* IconFocusCurve = nullptr;

	/** The material used for the button toggle plate. Note, all buttons may not have a toggle plate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Visuals Brush")
	UMaterialInterface* TogglePlateMaterial = nullptr;
};

/**
 * Structure containing data representing button audio assets and properties.
 */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtButtonAudioBrush
{
	GENERATED_BODY()

public:
	/** The sound which plays when a button is pressed. This sound is spatialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Audio Brush")
	USoundBase* PressedSound = nullptr;

	/** The sound which plays when a button is released. This sound is spatialized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Audio Brush")
	USoundBase* ReleasedSound = nullptr;
};

/**
 * Structure containing data representing categorized button assets and properties.
 */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtButtonBrush
{
	GENERATED_BODY()

public:
	/** Structure for button visuals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Brush")
	FUxtButtonVisualsBrush Visuals;

	/** Structure for button audio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Button Brush")
	FUxtButtonAudioBrush Audio;
};
