// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UxtTextBrush.generated.h"

class UFont;
class UMaterialInterface;

/**
 * Structure which contains data representing the appearance of text.
 */
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtTextBrush
{
	GENERATED_BODY()

public:
	/** The font used by the text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	UFont* Font = nullptr;

	/** The material used by the text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	UMaterialInterface* Material = nullptr;

	/** The text's location compared to it's parent component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	FVector RelativeLocation = FVector::ZeroVector;

	/** The text's rotation compared to it's parent component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	/** The size of the text, normally the font size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	float Size = 1.0f;

	/** The default color of the brush, by default the text color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	FColor DefaultColor = FColor::White;

	/** The disabled color of the brush, by default the text color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Text Brush")
	FColor DisabledColor = FColor(32, 32, 32);
};
