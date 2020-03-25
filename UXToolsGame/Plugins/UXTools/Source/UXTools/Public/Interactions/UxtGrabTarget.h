// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Input/UxtPointerTypes.h"
#include "UxtGrabTarget.generated.h"

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

	/** Raised when a pointer focuses the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEnterGrabFocus(int32 PointerId, const FUxtPointerInteractionData& Data);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateGrabFocus(int32 PointerId, const FUxtPointerInteractionData& Data);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitGrabFocus(int32 PointerId);

	/** Calculates the point on the target surface that is closest to the point passed in.
	 *  Return value indicates whether a point was found.
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool GetClosestGrabPoint(const FVector& Point, FVector& OutPointOnSurface) const;

	/** Raised when a pointer starts grabbing while overlapping the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnBeginGrab(int32 PointerId, const FUxtPointerInteractionData& Data);

	/** Raised when a pointer has been updated while grabbing. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateGrab(int32 PointerId, const FUxtPointerInteractionData& Data);

	/** Raised when a pointer stops grabbing or stops overlapping the actor while grabbing. */
	UFUNCTION(BlueprintNativeEvent)
	void OnEndGrab(int32 PointerId);
};
