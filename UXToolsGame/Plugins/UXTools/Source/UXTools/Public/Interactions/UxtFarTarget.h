// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtFarTarget.generated.h"

UINTERFACE(BlueprintType, Category = "UXTools")
class UXTOOLS_API UUxtFarTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface to implement to enable far interaction for given primitives. */
class UXTOOLS_API IUxtFarTarget
{
	GENERATED_BODY()

public:
	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Target")
	bool IsFarFocusable(const UPrimitiveComponent* Primitive) const;
};
