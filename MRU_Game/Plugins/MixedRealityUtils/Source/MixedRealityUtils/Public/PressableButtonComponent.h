// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InteractableComponent.h"

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
class UShapeComponent;
struct FButtonHandler;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonHoverStartDelegate, UPressableButtonComponent*, Button, USceneComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FButtonHoverEndDelegate, UPressableButtonComponent*, Button, USceneComponent*, Pointer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FButtonPressedDelegate, UPressableButtonComponent*, Button);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FButtonReleasedDelegate, UPressableButtonComponent*, Button);


/**
 * Component that turns the actor it is attached to into a pressable rectangular button.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXEDREALITYUTILS_API UPressableButtonComponent : public UInteractableComponent
{
	GENERATED_BODY()

public:	

	UPressableButtonComponent();

	/** Get scene component used for the moving visuals */
	UFUNCTION(BlueprintCallable)
	USceneComponent* GetVisuals() const;

	/** Set scene component to be used for the moving visuals */
	UFUNCTION(BlueprintCallable)
	void SetVisuals(USceneComponent* Visuals);

	/** Get shape component used as hover volume shape */
	UFUNCTION(BlueprintCallable)
	UShapeComponent* GetHoverVolumeShape() const;

	/** Set shape component to be used as hover volume shape */
	UFUNCTION(BlueprintCallable)
	void SetHoverVolumeShape(UShapeComponent* Shape);

protected:

    //
    // UActorComponent interface

	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//
	// ITouchPointerTarget interface

	virtual bool GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface) override;

public:

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

	/** Event raised when the first pointer enters the button collision box. */
	UPROPERTY(BlueprintAssignable)
	FButtonHoverStartDelegate OnButtonHoverStart;

	/** Event raised when the last pointer leaves the button collision box. */
	UPROPERTY(BlueprintAssignable)
	FButtonHoverEndDelegate OnButtonHoverEnd;

	/** Event raised when the button reaches the pressed distance. */
	UPROPERTY(BlueprintAssignable)
	FButtonPressedDelegate OnButtonPressed;

	/** Event raised when the a pressed button reaches the released distance. */
	UPROPERTY(BlueprintAssignable)
	FButtonReleasedDelegate OnButtonReleased;

private:

    Microsoft::MixedReality::HandUtils::PressableButton* Button = nullptr;
    FButtonHandler* ButtonHandler = nullptr;
	FVector VisualsPositionLocal;

	/** Visual representation of the button face. This component's transform will be updated as the button is pressed/released. */
	TWeakObjectPtr<USceneComponent> VisualsWeak;

	/** Shape defining the hover volume for the button. Pointers outside this volume will be ignored. */
	TWeakObjectPtr<UShapeComponent> HoverVolumeShapeWeak;
};
