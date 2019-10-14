// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableSceneComponent.h"

#include "PressableButtonComponent.generated.h"

namespace Microsoft
{
    namespace MixedReality
    {
        namespace HandUtils
        {
            class PressableButton;
        }
    }
}

class UPressableButtonComponent;
struct FButtonHandler;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FButtonPressedDelegate, UPressableButtonComponent*, Button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FButtonReleasedDelegate, UPressableButtonComponent*, Button);


/**
 * Component that turns the actor it is attached to into a pressable rectangular button.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXEDREALITYTOOLS_API UPressableButtonComponent : public UInteractableSceneComponent
{
	GENERATED_BODY()

public:	

	UPressableButtonComponent();

protected:

    //
    // UActorComponent interface

	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

	/** Button visuals. Will move as the button is pressed. */
	UPROPERTY(EditAnywhere)
	AActor* Visuals;

	/** 
	 * The extents (i.e. half the dimensions) of the button movement box.
	 * The X extent is the maximum travel distance for the button, Y and Z are the button width and height respectively.
	 */
	UPROPERTY(EditAnywhere)
	FVector Extents;

	/** Fraction of the maximum travel distance at which the button will raise the pressed event. */
    UPROPERTY(EditAnywhere)
    float PressedFraction;

	/** Fraction of the maximum travel distance at which a pressed button will raise the released event. */
    UPROPERTY(EditAnywhere)
    float ReleasedFraction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Pointer;

	/** Event raised when the button reaches the pressed distance. */
	UPROPERTY(BlueprintAssignable)
	FButtonPressedDelegate ButtonPressed;

	/** Event raised when the a pressed button reaches the released distance. */
	UPROPERTY(BlueprintAssignable)
	FButtonReleasedDelegate ButtonReleased;

private:

    Microsoft::MixedReality::HandUtils::PressableButton* Button = nullptr;
    FButtonHandler* ButtonHandler = nullptr;
	FVector VisualsPositionLocal;
};
