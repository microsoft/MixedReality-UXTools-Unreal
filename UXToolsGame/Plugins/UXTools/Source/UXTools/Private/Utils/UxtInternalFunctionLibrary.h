// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UxtInternalFunctionLibrary.generated.h"

class UFont;
class UTexture2D;
class UBlueprint;

/**
 * Blueprint structure which contains data required to render a character within a offline font.
 */
USTRUCT(BlueprintType, Category = "UXTools Internal")
struct UXTOOLS_API FUxtFontCharacter
{
	GENERATED_BODY()

	/**
	 * The single font character as a string.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Character")
	FString Text;

	/**
	* The texutre containing the font character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Character")
	UTexture2D* Texture;

	/**
	* The UV offset within the texture to find the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Character")
	FLinearColor UVTransform;

	/**
	* The normalized width and height of the character.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Font Character")
	FVector Size;
};

/**
 * Library of utility intenral functions for UX Tools.
 */
UCLASS()
class UXTOOLS_API UUxtInternalFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Converts a Unicode code point as hex into the corresponding UTF-16 FString representation. Returns true when the conversion is successful.*/
	UFUNCTION(BlueprintPure, Category = "UXTools Internal")
	static bool HexCodePointToFString(const FString& Input, FString& Output);

	/** Converts a UTF-16 FString into the corresponding unicode code point as hex representation. Returns true when the conversion is successful.*/
	UFUNCTION(BlueprintPure, Category = "UXTools Internal")
	static bool FStringToHexCodePoint(const FString& Input, FString& Output);

	/** Returns true if a UFont is using offline caching. */
	UFUNCTION(BlueprintPure, Category = "UXTools Internal")
	static bool IsFontOffline(const UFont* Font);

	/** Builds an array of FUxtFontCharacters for each character present in a UFont. */
	UFUNCTION(BlueprintPure, Category = "UXTools Internal")
	static bool GetFontCharacterData(const UFont* Font, TArray<FUxtFontCharacter>& FontCharacters);

	/** Returns whether the world the passed object belongs to is executing within a mobile preview window. */
	UFUNCTION(BlueprintPure, Category = "UXTools Internal")
	static bool IsPlayInMobilePreview(const UObject* WorldContextObject);
};
