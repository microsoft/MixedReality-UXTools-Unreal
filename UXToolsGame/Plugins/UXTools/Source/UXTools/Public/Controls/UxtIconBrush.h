// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtTextBrush.h"

#include "UxtIconBrush.generated.h"

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
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtIconBrush
{
	GENERATED_BODY()

public:
	/** String of characters representing a Unicode symbol, or a literal string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Icon Brush")
	FString Icon = "EBD2";

	/** Describes what the icon brush content represents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Icon Brush")
	EUxtIconBrushContentType ContentType = EUxtIconBrushContentType::UnicodeCharacter;

	/** Text settings for the icon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Icon Brush")
	FUxtTextBrush TextBrush;
};
