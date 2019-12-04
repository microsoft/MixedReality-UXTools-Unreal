// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TouchPointerTarget.generated.h"

class USceneComponent;

// This class does not need to be modified.
UINTERFACE(BlueprintType)
class UTouchPointerTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface a component must implement to interact with touch pointers. */
class MIXEDREALITYUTILS_API ITouchPointerTarget
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/** Raised by touch pointers when they start hovering this target. A given touch pointer can only hover one pointer at any given time. */
	UFUNCTION(BlueprintNativeEvent)
	void HoverStarted(UTouchPointer* pointer);

	/** Raised by touch pointers when they end hovering this target. */
	UFUNCTION(BlueprintNativeEvent)
	void HoverEnded(UTouchPointer* pointer);

	/** Raised by hovering touch pointers when they start grasping. */
	UFUNCTION(BlueprintNativeEvent)
	void GraspStarted(UTouchPointer* pointer);

	/** Raised by hovering touch pointers when they end grasping. */
	UFUNCTION(BlueprintNativeEvent)
	void GraspEnded(UTouchPointer* pointer);

	/** Calculates the point on the target surface that is closest to the point passed in. Return value indicates whether a point was found. */
	UFUNCTION(BlueprintNativeEvent)
	bool GetClosestPointOnSurface(const FVector& Point, FVector& OutPointOnSurface);
};
