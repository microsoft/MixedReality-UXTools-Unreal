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

/**
 * 
 */
class MIXEDREALITYTOOLS_API ITouchPointerTarget
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent)
	void TouchStarted(UTouchPointer* pointer);

	UFUNCTION(BlueprintNativeEvent)
	void TouchEnded(UTouchPointer* pointer);

	/** Calculates the point on the target surface that is closest to the point passed in. Return value indicates whether a point was found. */
	UFUNCTION(BlueprintNativeEvent)
	bool GetClosestPointOnSurface(const FVector& Point, FVector& OutPointOnSurface);
};
