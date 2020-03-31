// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UxtTouchTarget.generated.h"

class UUxtNearPointerComponent;

UENUM(BlueprintType)
enum class EUxtTouchBehaviour : uint8
{
	/** Target represents a plane, only touchable from the front face */
	FrontFace,
	/** Target represents a mesh volume, touchable from all sides */
	Volume,
};

UINTERFACE(BlueprintType)
class UUxtTouchTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be touched. */
class UXTOOLS_API IUxtTouchTarget
{
	GENERATED_BODY()

public:

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEnterTouchFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateTouchFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitTouchFocus(UUxtNearPointerComponent* Pointer);

	/** Returns which touch behaviour this target supports. */
	UFUNCTION(BlueprintNativeEvent)
	EUxtTouchBehaviour GetTouchBehaviour() const;

	/** Raised when a pointer touch volume starts overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnBeginTouch(UUxtNearPointerComponent* Pointer);

	/** Raised while a pointer touch volume is overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateTouch(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer touch volume stops overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEndTouch(UUxtNearPointerComponent* Pointer);
};
