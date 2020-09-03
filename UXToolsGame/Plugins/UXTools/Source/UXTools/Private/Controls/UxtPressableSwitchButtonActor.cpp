// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableSwitchButtonActor.h"

#include "Components/TextRenderComponent.h"
#include "Controls/UxtToggleStateComponent.h"

AUxtPressableSwitchButtonActor::AUxtPressableSwitchButtonActor()
{
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
