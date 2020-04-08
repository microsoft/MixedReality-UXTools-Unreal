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

/** Interface to be implemented by components to handle far interactions. */
class UXTOOLS_API IUxtFarTarget
{
	GENERATED_BODY()

public:

	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent)
	bool IsFarFocusable(const UPrimitiveComponent* Primitive);

	/** Raised when a far pointer starts focusing a primitive. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEnterFarFocus(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is updated. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdatedFarFocus(UUxtFarPointerComponent* Pointer);

	/** Raised when a far pointer stops focusing a primitive. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitFarFocus(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is pressed. */
	UFUNCTION(BlueprintNativeEvent)
	void OnFarPressed(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is dragged. */
	UFUNCTION(BlueprintNativeEvent)
	void OnFarDragged(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is released. */
	UFUNCTION(BlueprintNativeEvent)
	void OnFarReleased(UUxtFarPointerComponent* Pointer);
};
