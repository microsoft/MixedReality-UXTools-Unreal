// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtGrabTarget.generated.h"

class UUxtNearPointerComponent;

UINTERFACE(BlueprintType)
class UUxtGrabTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface to implement to enable grab interaction for given primitives. */
class UXTOOLS_API IUxtGrabTarget
{
	GENERATED_BODY()

public:
	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent)
	bool IsGrabFocusable(const UPrimitiveComponent* Primitive) const;
};
