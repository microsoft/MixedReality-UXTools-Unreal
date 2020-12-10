// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Interactions/UxtGrabTargetComponent.h"

#include "GrabTickTestComponent.generated.h"

UCLASS(ClassGroup = "UXToolsTests")
class UXTOOLSTESTS_API UGrabTickTestComponent : public UUxtGrabTargetComponent
{
	GENERATED_BODY()

public:
	UGrabTickTestComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
		PrimaryComponentTick.bStartWithTickEnabled = false;
		bAutoActivate = true;
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override { ++NumTicks; }

	int GetNumTicks() const { return NumTicks; }

	void Reset() { NumTicks = 0; }

private:
	int NumTicks = 0;
};
