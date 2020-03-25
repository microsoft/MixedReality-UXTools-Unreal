// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Input/UxtPointerTypes.h"
#include "UxtTouchTarget.generated.h"

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
	void OnEnterTouchFocus(int32 PointerId, const FUxtPointerInteractionData& Data);

	/** Raised when a pointer has been updated while focused. */
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateTouchFocus(int32 PointerId, const FUxtPointerInteractionData& Data);

	/** Raised when a pointer stops focusing the actor. */
	UFUNCTION(BlueprintNativeEvent)
	void OnExitTouchFocus(int32 PointerId);

	/** Calculates the point on the target surface that is closest to the point passed in.
	 *  Return value indicates whether a point was found.
	 */
	UFUNCTION(BlueprintNativeEvent)
	bool GetClosestTouchPoint(const FVector& Point, FVector& OutPointOnSurface) const;

	// TODO have no functional general touch detection mechanism yet
	///** Raised when a pointer touch volume starts overlapping the actor. */
	//UFUNCTION(BlueprintNativeEvent)
	//void OnBeginTouch(int32 PointerId);

	///** Raised when a pointer touch volume stops overlapping the actor. */
	//UFUNCTION(BlueprintNativeEvent)
	//void OnEndTouch(int32 PointerId);
};
