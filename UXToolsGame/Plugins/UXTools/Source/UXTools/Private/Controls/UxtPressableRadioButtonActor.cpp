// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableRadioButtonActor.h"

#include "UxTools.h"

#include "Components/TextRenderComponent.h"
#include "Controls/UxtToggleStateComponent.h"
#include "Engine/Font.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtInternalFunctionLibrary.h"

AUxtPressableRadioButtonActor::AUxtPressableRadioButtonActor()
{
	// Load the default assets.
	static ConstructorHelpers::FObjectFinder<UFont> DefaultIconFont(
		TEXT("Font'/UXTools/Fonts/Font_SegoeMDL2_Regular_42.Font_SegoeMDL2_Regular_42'"));
	check(DefaultIconFont.Object);

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultTextMaterial(TEXT("Material'/UXTools/Fonts/M_DefaultFont.M_DefaultFont'"));
	check(DefaultTextMaterial.Object);

	// Set the default button label.
	Label = NSLOCTEXT("UxtPressableButtonActor", "LabelRadio", "Radio");

	// Setup the default icon.
	IconBrush.Icon = "F126";
	IconBrush.TextBrush.Font = DefaultIconFont.Object;
	IconBrush.TextBrush.Material = DefaultTextMaterial.Object;

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
	CenterIconComponent->SetText(FText::AsCultureInvariant(Output));

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
