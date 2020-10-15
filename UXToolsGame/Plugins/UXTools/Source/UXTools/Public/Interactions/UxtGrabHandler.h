// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtGrabHandler.generated.h"

class UUxtNearPointerComponent;
class UPrimitiveComponent;

UINTERFACE(BlueprintType)
class UUxtGrabHandler : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be grabbed. */
class UXTOOLS_API IUxtGrabHandler
{
	GENERATED_BODY()

public:
	/** Returns true if the this can handle events from this primitive. */
	UFUNCTION(BlueprintNativeEvent)
	bool CanHandleGrab(UPrimitiveComponent* Primitive) const;

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
