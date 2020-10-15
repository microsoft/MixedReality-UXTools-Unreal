// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtPokeHandler.generated.h"

class UUxtNearPointerComponent;
class UPrimitiveComponent;

UINTERFACE(BlueprintType)
class UUxtPokeHandler : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be poked. */
class UXTOOLS_API IUxtPokeHandler
{
	GENERATED_BODY()

public:
	/** Returns true if the this can handle events from this primitive. */
	UFUNCTION(BlueprintNativeEvent)
	bool CanHandlePoke(UPrimitiveComponent* Primitive) const;

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEnterPokeFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdatePokeFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitPokeFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer poke volume starts overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnBeginPoke(UUxtNearPointerComponent* Pointer);

	/** Raised while a pointer poke volume is overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdatePoke(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer poke volume stops overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEndPoke(UUxtNearPointerComponent* Pointer);
};
