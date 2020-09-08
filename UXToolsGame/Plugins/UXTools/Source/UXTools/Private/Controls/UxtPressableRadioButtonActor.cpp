// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableRadioButtonActor.h"

#include "UxTools.h"

#include "Components/TextRenderComponent.h"
#include "Controls/UxtToggleStateComponent.h"
#include "Utils/UxtInternalFunctionLibrary.h"

AUxtPressableRadioButtonActor::AUxtPressableRadioButtonActor()
{
	// Create the component hierarchy.
	CenterIconComponent = CreateAndAttachComponent<UTextRenderComponent>(TEXT("CenterIcon"), IconComponent);
	CenterIconComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	CenterIconComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	CenterIconComponent->SetWorldSize(0.65f);

	FString Input = TEXT("EC14");
	FString Output;
	const bool Result = UUxtInternalFunctionLibrary::HexCodePointToFString(Input, Output);
	UE_CLOG(
		!Result, UXTools, Warning, TEXT("Failed to resolve hex code point '%s' on AUxtPressableRadioButtonActor '%s'."), *Input,
		*GetName());
	CenterIconComponent->SetText(FText::FromString(Output));

	RemoveTogglePlate();
}

void AUxtPressableRadioButtonActor::ConstructIcon()
{
	Super::ConstructIcon();

	CenterIconComponent->SetFont(IconBrush.TextBrush.Font);
	CenterIconComponent->SetMaterial(0, IconBrush.TextBrush.Material);
	CenterIconComponent->SetTextRenderColor(IconBrush.TextBrush.DefaultColor);
}

void AUxtPressableRadioButtonActor::UpdateToggleVisuals()
{
	Super::UpdateToggleVisuals();

	IconComponent->SetTextRenderColor(ToggleStateComponent->IsChecked() ? FColor(0, 95, 128) : IconBrush.TextBrush.DefaultColor);
	CenterIconComponent->SetVisibility(ToggleStateComponent->IsChecked());
}
