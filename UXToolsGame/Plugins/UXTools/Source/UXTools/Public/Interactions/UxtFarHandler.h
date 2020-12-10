// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtFarHandler.generated.h"

class UPrimitiveComponent;
class UUxtFarPointerComponent;

UINTERFACE(BlueprintType, Category = "UXTools")
class UXTOOLS_API UUxtFarHandler : public UInterface
{
	GENERATED_BODY()
};

/** Interface to be implemented by components to handle far interactions. */
class UXTOOLS_API IUxtFarHandler
{
	GENERATED_BODY()

public:
	/** Returns true if the this can handle events from this primitive. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	bool CanHandleFar(UPrimitiveComponent* Primitive) const;

	/** Raised when a far pointer starts focusing a primitive. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	void OnEnterFarFocus(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is updated. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	void OnUpdatedFarFocus(UUxtFarPointerComponent* Pointer);

	/** Raised when a far pointer stops focusing a primitive. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	void OnExitFarFocus(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is pressed. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	void OnFarPressed(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is dragged. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	void OnFarDragged(UUxtFarPointerComponent* Pointer);

	/** Raised when a focusing far pointer is released. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Far Handler")
	void OnFarReleased(UUxtFarPointerComponent* Pointer);
};
