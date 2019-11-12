// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MixedRealityHand.generated.h"

class UHandController;
class UTouchPointer;

/**
 * Representation of a hand for interaction with other actors.
 */
UCLASS()
class MIXEDREALITYTOOLS_API AMixedRealityHand : public AActor
{
	GENERATED_BODY()

public:

	AMixedRealityHand();

	UFUNCTION(BlueprintCallable)
	void SetTouchPointerWorldLocation(FVector location);

	UFUNCTION(BlueprintCallable)
	void SetTouchPointerRelativeLocation(FVector location);

	UFUNCTION(BlueprintCallable)
	void DebugDraw();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EControllerHand Handedness;

protected:

	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UTouchPointer *TouchPointer;

	FColor GetColor() const;

};
