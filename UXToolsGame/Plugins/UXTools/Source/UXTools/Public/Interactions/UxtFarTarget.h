// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UxtFarTarget.generated.h"


UINTERFACE(BlueprintType)
class UUxtFarTarget : public UInterface
{
	GENERATED_BODY()
};

class UUxtFarPointerComponent;

class UXTOOLS_API IUxtFarTarget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	void OnEnterFarFocus(UUxtFarPointerComponent* Pointer);

	UFUNCTION(BlueprintNativeEvent)
	void OnUpdatedFarFocus(UUxtFarPointerComponent* Pointer);

	UFUNCTION(BlueprintNativeEvent)
	void OnExitFarFocus(UUxtFarPointerComponent* Pointer);

	UFUNCTION(BlueprintNativeEvent)
	void OnFarPressed(UUxtFarPointerComponent* Pointer);

	UFUNCTION(BlueprintNativeEvent)
	void OnFarDragged(UUxtFarPointerComponent* Pointer);

	UFUNCTION(BlueprintNativeEvent)
	void OnFarReleased(UUxtFarPointerComponent* Pointer);
};
