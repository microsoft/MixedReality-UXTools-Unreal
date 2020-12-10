// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/TextRenderComponent.h"

#include "UxtTextRenderComponent.generated.h"

/**
 * A text render component which automatically configures assets and properties best suited for text rendering in UX Tools.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTextRenderComponent : public UTextRenderComponent
{
	GENERATED_BODY()

public:
	UUxtTextRenderComponent();
};
