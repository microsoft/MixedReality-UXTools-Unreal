// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtGrabHandler.generated.h"

class UUxtNearPointerComponent;
class UPrimitiveComponent;

UINTERFACE(BlueprintType, Category = "UXTools")
class UXTOOLS_API UUxtGrabHandler : public UInterface
{
	GENERATED_BODY()
};

/** Interface for components that can be grabbed. */
class UXTOOLS_API IUxtGrabHandler
{
	GENERATED_BODY()

public:
	/** Returns true if the this can handle events from this primitive. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	bool CanHandleGrab(UPrimitiveComponent* Primitive) const;

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	void OnEnterGrabFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	void OnUpdateGrabFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	void OnExitGrabFocus(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer starts grabbing while overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	void OnBeginGrab(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer has been updated while grabbing. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	void OnUpdateGrab(UUxtNearPointerComponent* Pointer);

	/** Raised when a pointer stops grabbing or stops overlapping the actor while grabbing. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Grab Handler")
	void OnEndGrab(UUxtNearPointerComponent* Pointer);
};
