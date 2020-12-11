// Copyright (c) 2020 Microsoft Corporation.
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
USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtFontCharacter
{
	GENERATED_BODY()

	/**
	 * The single font character as a string.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Font Character")
	FString Text;

	/**
	 * The texutre containing the font character.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Font Character")
	UTexture2D* Texture;

	/**
	 * The UV offset within the texture to find the character.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Font Character")
	FLinearColor UVTransform;

	/**
	 * The normalized width and height of the character.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Font Character")
	FVector Size;
};

/**
 * Library of utility internal functions for UX Tools.
 */
UCLASS(ClassGroup = "UXTools|Internal")
class UXTOOLS_API UUxtInternalFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Converts a Unicode code point as hex into the corresponding UTF-16 FString representation. Returns true when the conversion is
	 * successful.*/
	UFUNCTION(BlueprintPure, Category = "UXTools|Internal")
	static bool HexCodePointToFString(const FString& Input, FString& Output);

	/** Converts a UTF-16 FString into the corresponding unicode code point as hex representation. Returns true when the conversion is
	 * successful.*/
	UFUNCTION(BlueprintPure, Category = "UXTools|Internal")
	static bool FStringToHexCodePoint(const FString& Input, FString& Output);

	/** Returns true if a UFont is using offline caching. */
	UFUNCTION(BlueprintPure, Category = "UXTools|Internal")
	static bool IsFontOffline(const UFont* Font);

	/** Builds an array of FUxtFontCharacters for each character present in a UFont. */
	UFUNCTION(BlueprintPure, Category = "UXTools|Internal")
	static bool GetFontCharacterData(const UFont* Font, TArray<FUxtFontCharacter>& FontCharacters);

	/** Spherical linear interpolate between two vectors */
	UFUNCTION(BlueprintPure, Category = "UXTools|Internal")
	static FVector Slerp(const FVector& Vector1, const FVector& Vector2, const float Slerp);

	/** Get the object from a TSoftObjectPtr, loading it synchronously if it is not loaded. */
	template <typename T>
	static T* GetObjectFromPtr(const TSoftObjectPtr<T>& ObjectPtr)
	{
		if (ObjectPtr.IsPending())
		{
			ObjectPtr.LoadSynchronous();
		}

		return ObjectPtr.Get();
	}
};
