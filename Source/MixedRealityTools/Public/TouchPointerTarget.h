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
    void TouchStarted(USceneComponent* pointer);

    UFUNCTION(BlueprintNativeEvent)
    void TouchEnded(USceneComponent* pointer);

};
