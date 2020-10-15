// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtCollectionObject.generated.h"

class UUxtPokeTarget;
class IUxtFarTarget;

UINTERFACE(BlueprintType)
class UUxtCollectionObject : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be poked. */
class UXTOOLS_API IUxtCollectionObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	TScriptInterface<IUxtPokeTarget> GetPokeTarget();

	UFUNCTION(BlueprintNativeEvent)
	TScriptInterface<IUxtFarTarget> GetFarTarget();
};
