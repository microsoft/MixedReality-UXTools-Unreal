// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Utils/UxtInternalFunctionLibrary.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"

bool UUxtInternalFunctionLibrary::HexCodePointToFString(const FString& Input, FString& Output)
{
	TCHAR Char = (TCHAR)FCString::Strtoi(*Input, nullptr, 16);
	if (Char != 0)
	{
		Output.Reset(1);
		Output.AppendChar(Char);
		return true;
	}

	Output = Input;
	return false;
}

bool UUxtInternalFunctionLibrary::FStringToHexCodePoint(const FString& Input, FString& Output)
{
	if (Input.Len() != 0)
	{
		Output = FString::Printf(TEXT("%04X"), Input[0]);

		return true;
	}

	Output = Input;
	return false;
}

bool UUxtInternalFunctionLibrary::IsFontOffline(const UFont* Font)
{
	return (Font != nullptr) && (Font->FontCacheType == EFontCacheType::Offline);
}

void AddCharacter(const FFontCharacter& Character, uint16 CodePoint, const UFont* Font, TArray<FUxtFontCharacter>& FontCharacters)
{
	// If the character's size is 0, it's non-printable.
	if (Character.VSize == 0)
	{
		return;
	}

	FUxtFontCharacter FontCharacter;
	FontCharacter.Text.AppendChar(CodePoint);

	if (Font->Textures.IsValidIndex(Character.TextureIndex))
	{
		FontCharacter.Texture = Font->Textures[Character.TextureIndex];

		if (FontCharacter.Texture != nullptr)
		{
			FIntPoint ImportedTextureSize = FontCharacter.Texture->GetImportedSize();
			FVector2D InvTextureSize(1.0f / (float)ImportedTextureSize.X, 1.0f / (float)ImportedTextureSize.Y);

			const float OffsetU = Character.StartU * InvTextureSize.X;
			const float OffsetV = Character.StartV * InvTextureSize.Y;
			const float ScaleU = Character.USize * InvTextureSize.X;
			const float ScaleV = Character.VSize * InvTextureSize.Y;

			FontCharacter.UVTransform = FLinearColor(OffsetU, OffsetV, ScaleU, ScaleV);

			float Width, Height = 0;
			Font->GetCharSize(CodePoint, Width, Height);
			float MaxSize = FMath::Max(Width, Height);
			MaxSize = (MaxSize == 0) ? 1 : MaxSize;

			FontCharacter.Size = FVector(Width / MaxSize, Height / MaxSize, 0);
		}
	}

	FontCharacters.Add(FontCharacter);
}

bool UUxtInternalFunctionLibrary::GetFontCharacterData(const UFont* Font, TArray<FUxtFontCharacter>& FontCharacters)
{
	if (!IsFontOffline(Font))
	{
		return false;
	}

	FontCharacters.Empty();

	if (Font->IsRemapped)
	{
		for (auto& Elem : Font->CharRemap)
		{
			AddCharacter(Font->Characters[Elem.Value], Elem.Key, Font, FontCharacters);
		}
	}
	else
	{
		for (int32 Idx = 0; Idx < Font->Characters.Num(); ++Idx)
		{
			AddCharacter(Font->Characters[Idx], Idx, Font, FontCharacters);
		}
	}

	return true;
}

bool UUxtInternalFunctionLibrary::IsPlayInMobilePreview(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
		{
			return World->IsPlayInMobilePreview();
		}
	}

	return false;
}
