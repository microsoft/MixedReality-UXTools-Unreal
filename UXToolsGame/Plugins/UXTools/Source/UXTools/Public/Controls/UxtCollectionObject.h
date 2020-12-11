// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtCollectionObject.generated.h"

class UUxtPokeTarget;
class IUxtFarTarget;

UINTERFACE(BlueprintType)
class UXTOOLS_API UUxtCollectionObject : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be poked. */
class UXTOOLS_API IUxtCollectionObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Collection Object")
	TScriptInterface<IUxtPokeTarget> GetPokeTarget();

	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Collection Object")
	TScriptInterface<IUxtFarTarget> GetFarTarget();
};
