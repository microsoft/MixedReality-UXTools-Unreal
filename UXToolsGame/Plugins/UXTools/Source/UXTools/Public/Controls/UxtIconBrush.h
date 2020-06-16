// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtIconBrush.generated.h"

class UFont;

UENUM(BlueprintType)
enum class EUxtIconBrushContentType : uint8
{
	/** The icon brush has no content. */
	None,
	/** The icon brush content represents a Unicode character. */
	UnicodeCharacter,
	/** The icon brush content represents a string. */
	String
};

/**
 * Structure which contains data representing an icon's appearance. Icons are currently represented as
 * Unicode characters or strings, but in the future could be textures, models, etc.
 */
USTRUCT(BlueprintType, Category = "UXTools")
struct UXTOOLS_API FUxtIconBrush
{
	GENERATED_BODY()

	/** Describes what the icon brush content represents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon Brush")
	EUxtIconBrushContentType IconContentType;

	/** The font used by the IconString. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon Brush")
	UFont* IconFont;

	/** String of characters representing a Unicode symbol, or a literal string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon Brush")
	FString IconString;
};
