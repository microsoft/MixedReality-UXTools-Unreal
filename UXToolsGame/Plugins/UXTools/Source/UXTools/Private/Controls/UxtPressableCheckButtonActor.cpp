// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtPressableCheckButtonActor.h"

#include "Controls/UxtToggleStateComponent.h"

AUxtPressableCheckButtonActor::AUxtPressableCheckButtonActor()
{
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
