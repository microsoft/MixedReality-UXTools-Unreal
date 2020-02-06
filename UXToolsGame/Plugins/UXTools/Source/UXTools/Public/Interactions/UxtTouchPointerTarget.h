// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UxtTouchPointerTarget.generated.h"

class USceneComponent;
class UUxtTouchPointer;

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UUxtTouchPointerTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface a component must implement to interact with touch pointers. */
class UXTOOLS_API IUxtTouchPointerTarget
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/** Raised by touch pointers when they start hovering this target. A given touch pointer can only hover one pointer at any given time. */
	UFUNCTION(BlueprintNativeEvent)
	void HoverStarted(UUxtTouchPointer* pointer);

	/** Raised by touch pointers when they end hovering this target. */
	UFUNCTION(BlueprintNativeEvent)
	void HoverEnded(UUxtTouchPointer* pointer);

	/** Raised by hovering touch pointers when they start grasping. */
	UFUNCTION(BlueprintNativeEvent)
	void GraspStarted(UUxtTouchPointer* pointer);

	/** Raised by hovering touch pointers when they end grasping. */
	UFUNCTION(BlueprintNativeEvent)
	void GraspEnded(UUxtTouchPointer* pointer);

	/** Calculates the point on the target surface that is closest to the point passed in. Return value indicates whether a point was found. */
	UFUNCTION(BlueprintNativeEvent)
	bool GetClosestPointOnSurface(const FVector& Point, FVector& OutPointOnSurface);
};
