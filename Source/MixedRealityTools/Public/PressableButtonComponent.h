// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PressableButton.h"
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


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MIXEDREALITYTOOLS_API UPressableButtonComponent : public UActorComponent
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

private:

    struct FButtonHandler : public Microsoft::MixedReality::HandUtils::IButtonHandler
    {
        FButtonHandler(UPressableButtonComponent& PressableButtonComponent) : PressableButtonComponent(PressableButtonComponent) {}

        virtual void OnButtonPressed(
            Microsoft::MixedReality::HandUtils::PressableButton& button, 
            Microsoft::MixedReality::HandUtils::PointerId pointerId, 
            DirectX::FXMVECTOR touchPoint) override;

        UPressableButtonComponent& PressableButtonComponent;
    };

    friend FButtonHandler;

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

    DECLARE_EVENT_OneParam(UPressableButtonComponent, FPressedEvent, UPressableButtonComponent&)
    FPressedEvent& OnPressed() { return PressedEvent; }

private:

    Microsoft::MixedReality::HandUtils::PressableButton* Button = nullptr;
    FButtonHandler* ButtonHandler = nullptr;

    FPressedEvent PressedEvent;
};
