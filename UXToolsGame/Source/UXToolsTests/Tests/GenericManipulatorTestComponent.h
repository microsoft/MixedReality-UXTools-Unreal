// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Misc/AutomationTest.h"

#include "GenericManipulatorTestComponent.generated.h"

class UUxtGenericManipulatorComponent;

/**
 * Target for generic manipulator tests that counts events.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UGenericManipulatorTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void UpdateTransform(USceneComponent* TargetComponent, FTransform Transform) { TransformUpdateCount++; }

public:
	int TransformUpdateCount = 0;
};
