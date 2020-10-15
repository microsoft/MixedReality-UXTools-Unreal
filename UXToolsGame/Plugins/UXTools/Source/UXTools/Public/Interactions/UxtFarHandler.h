// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtFarHandler.generated.h"

class UPrimitiveComponent;
class UUxtFarPointerComponent;

UINTERFACE(BlueprintType)
class UUxtFarHandler : public UInterface
{
	GENERATED_BODY()
};

/** Interface to be implemented by components to handle far interactions. */
class UXTOOLS_API IUxtFarHandler
{
	GENERATED_BODY()

public:
	/** Returns true if the this can handle events from this primitive. */
	UFUNCTION(BlueprintNativeEvent)
	bool CanHandleFar(UPrimitiveComponent* Primitive) const;

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
