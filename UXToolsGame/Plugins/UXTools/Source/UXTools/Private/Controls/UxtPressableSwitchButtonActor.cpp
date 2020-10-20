// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableSwitchButtonActor.h"

#include "Components/TextRenderComponent.h"
#include "Controls/UxtToggleStateComponent.h"
#include "Engine/Font.h"
#include "Materials/Material.h"
#include "UObject/ConstructorHelpers.h"

AUxtPressableSwitchButtonActor::AUxtPressableSwitchButtonActor()
{
	// Load the default assets.
	static ConstructorHelpers::FObjectFinder<UFont> DefaultIconFont(
		TEXT("Font'/UXTools/Fonts/Font_SegoeMDL2_Regular_42.Font_SegoeMDL2_Regular_42'"));
	check(DefaultIconFont.Object);

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultTextMaterial(TEXT("Material'/UXTools/Fonts/M_DefaultFont.M_DefaultFont'"));
	check(DefaultTextMaterial.Object);

	// Set the default button label.
	Label = NSLOCTEXT("UxtPressableButtonActor", "LabelSwitch", "Switch");

	// Setup the default switched off icon.
	SwitchedOnIconBrush.Icon = "EC11";
	SwitchedOnIconBrush.TextBrush.Font = DefaultIconFont.Object;
	SwitchedOnIconBrush.TextBrush.Material = DefaultTextMaterial.Object;
	SwitchedOnIconBrush.TextBrush.Size = 1.4f;

	// Setup the default switched on icon.
	SwitchedOffIconBrush.Icon = "EC12";
	SwitchedOffIconBrush.TextBrush.Font = DefaultIconFont.Object;
	SwitchedOffIconBrush.TextBrush.Material = DefaultTextMaterial.Object;
	SwitchedOffIconBrush.TextBrush.Size = 1.4f;

	bCanEditIconBrush = false;

	CenterIconComponent->SetWorldSize(0.4f);

	RemoveTogglePlate();
}

void AUxtPressableSwitchButtonActor::UpdateToggleVisuals()
{
	// Re-construct the icon first to avoid side-effects by the super class.
	IconBrush = ToggleStateComponent->IsChecked() ? SwitchedOnIconBrush : SwitchedOffIconBrush;
	ConstructIcon();

	Super::UpdateToggleVisuals();

	// The super class will disabled the center icon. For the switch we want it always visible.
	CenterIconComponent->SetVisibility(true);
	CenterIconComponent->SetRelativeLocation(FVector(0.01f, 0.4f * (ToggleStateComponent->IsChecked() ? -1 : 1), 0));
}

void AUxtPressableSwitchButtonActor::SetSwitchedOffIconBrush(const FUxtIconBrush& Brush)
{
	SwitchedOffIconBrush = Brush;
	UpdateToggleVisuals();
}

void AUxtPressableSwitchButtonActor::SetSwitchedOnIconBrush(const FUxtIconBrush& Brush)
{
	SwitchedOnIconBrush = Brush;
	UpdateToggleVisuals();
}
