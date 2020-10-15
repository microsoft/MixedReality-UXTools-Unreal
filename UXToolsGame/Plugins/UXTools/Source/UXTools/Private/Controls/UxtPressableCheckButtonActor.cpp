// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableCheckButtonActor.h"

#include "Controls/UxtToggleStateComponent.h"
#include "Engine/Font.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

AUxtPressableCheckButtonActor::AUxtPressableCheckButtonActor()
{
	// Load the default assets.
	static ConstructorHelpers::FObjectFinder<UFont> DefaultIconFont(
		TEXT("Font'/UXTools/Fonts/Font_SegoeMDL2_Regular_42.Font_SegoeMDL2_Regular_42'"));
	check(DefaultIconFont.Object);

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultTextMaterial(TEXT("Material'/UXTools/Fonts/M_DefaultFont.M_DefaultFont'"));
	check(DefaultTextMaterial.Object);

	// Set the default button label.
	Label = NSLOCTEXT("UxtPressableButtonActor", "LabelCheck", "Check");

	// Setup the default checked icon.
	CheckedIconBrush.Icon = "E73D";
	CheckedIconBrush.TextBrush.Font = DefaultIconFont.Object;
	CheckedIconBrush.TextBrush.Material = DefaultTextMaterial.Object;

	// Setup the default unchecked icon.
	UncheckedIconBrush.Icon = "E739";
	UncheckedIconBrush.TextBrush.Font = DefaultIconFont.Object;
	UncheckedIconBrush.TextBrush.Material = DefaultTextMaterial.Object;

	bCanEditIconBrush = false;

	RemoveTogglePlate();
}

void AUxtPressableCheckButtonActor::UpdateToggleVisuals()
{
	Super::UpdateToggleVisuals();

	IconBrush = ToggleStateComponent->IsChecked() ? CheckedIconBrush : UncheckedIconBrush;
	ConstructIcon();
}

void AUxtPressableCheckButtonActor::SetUncheckedIconBrush(const FUxtIconBrush& Brush)
{
	UncheckedIconBrush = Brush;
	UpdateToggleVisuals();
}

void AUxtPressableCheckButtonActor::SetCheckedIconBrush(const FUxtIconBrush& Brush)
{
	CheckedIconBrush = Brush;
	UpdateToggleVisuals();
}
