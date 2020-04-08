// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UxtPokeTarget.generated.h"

class UPrimitiveComponent;
class UUxtNearPointerComponent;

UENUM(BlueprintType)
enum class EUxtPokeBehaviour : uint8
{
	/** Target represents a plane, only pokable from the front face */
	FrontFace,
	/** Target represents a mesh volume, pokable from all sides */
	Volume,
};

UINTERFACE(BlueprintType)
class UUxtPokeTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be poked. */
class UXTOOLS_API IUxtPokeTarget
{
	GENERATED_BODY()

public:

	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent)
	bool IsPokeFocusable(const UPrimitiveComponent* Primitive);

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEnterPokeFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdatePokeFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitPokeFocus(UUxtNearPointerComponent* Pointer);

	/** Returns which poke behaviour this target supports. */
	UFUNCTION(BlueprintNativeEvent)
	EUxtPokeBehaviour GetPokeBehaviour() const;

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
