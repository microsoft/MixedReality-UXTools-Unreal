// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtToggleStateComponent.h"

void UUxtToggleStateComponent::SetIsChecked(bool IsChecked)
{
	if (bIsChecked != IsChecked)
	{
		bIsChecked = IsChecked;
		OnToggled.Broadcast(this);
	}
}
