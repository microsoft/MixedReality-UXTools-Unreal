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

    UPROPERTY(EditAnywhere)
    float Width;

    UPROPERTY(EditAnywhere)
    float Height;

    UPROPERTY(EditAnywhere)
    float MaxPushDistance; 
    
    UPROPERTY(EditAnywhere)
    float PressedDistance;

    UPROPERTY(EditAnywhere)
    float ReleasedDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Pointer;

	UPROPERTY(BlueprintAssignable)
	FButtonPressedDelegate ButtonPressed;

	UPROPERTY(BlueprintAssignable)
	FButtonReleasedDelegate ButtonReleased;

private:

    Microsoft::MixedReality::HandUtils::PressableButton* Button = nullptr;
    FButtonHandler* ButtonHandler = nullptr;
};
