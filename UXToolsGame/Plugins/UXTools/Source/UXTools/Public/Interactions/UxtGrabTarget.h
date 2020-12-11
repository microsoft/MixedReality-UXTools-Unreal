// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtGrabTarget.generated.h"

class UUxtNearPointerComponent;

UINTERFACE(BlueprintType, Category = "UXTools")
class UXTOOLS_API UUxtGrabTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface to implement to enable grab interaction for given primitives. */
class UXTOOLS_API IUxtGrabTarget
{
	GENERATED_BODY()

public:
	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Target")
	bool IsGrabFocusable(const UPrimitiveComponent* Primitive) const;
};
