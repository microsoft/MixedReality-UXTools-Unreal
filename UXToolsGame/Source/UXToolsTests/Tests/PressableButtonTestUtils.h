// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Controls/UxtPressableButtonComponent.h"

/** Utility function to sets pressable button visuals for use in automation tests */
USceneComponent* SetTestButtonVisuals(UUxtPressableButtonComponent* Button, EUxtPushBehavior PushBehavior = EUxtPushBehavior::Translate);

/** Utility function that returns a new button component for testing */
UUxtPressableButtonComponent* CreateTestButtonComponent(
	AActor* Actor, const FVector& Location, EUxtPushBehavior PushBehavior = EUxtPushBehavior::Translate);
