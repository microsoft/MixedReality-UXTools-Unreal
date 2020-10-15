// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtPointerComponent.h"

bool UUxtPointerComponent::GetFocusLocked() const
{
	return bFocusLocked;
}

void UUxtPointerComponent::SetFocusLocked(bool bLocked)
{
	bFocusLocked = bLocked;
}
