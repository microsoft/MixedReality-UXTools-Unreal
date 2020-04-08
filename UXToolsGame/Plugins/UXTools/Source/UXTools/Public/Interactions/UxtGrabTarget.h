// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UxtGrabTarget.generated.h"

class UUxtNearPointerComponent;

UINTERFACE(BlueprintType)
class UUxtGrabTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be grabbed. */
class UXTOOLS_API IUxtGrabTarget
{
	GENERATED_BODY()

public:

	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent)
	bool IsGrabFocusable(const UPrimitiveComponent* Primitive);

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEnterGrabFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateGrabFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitGrabFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer starts grabbing while overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnBeginGrab(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while grabbing. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateGrab(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops grabbing or stops overlapping the actor while grabbing. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEndGrab(UUxtNearPointerComponent* Pointer);
};
