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
class MIXEDREALITYUTILS_API AMixedRealityHand : public AActor
{
	GENERATED_BODY()

public:

	AMixedRealityHand();

	UFUNCTION(BlueprintCallable, Category = "Mixed Reality Hand")
	void SetTouchPointerWorldLocation(FVector location);

	UFUNCTION(BlueprintCallable, Category = "Mixed Reality Hand")
	void SetTouchPointerRelativeLocation(FVector location);

	UFUNCTION(BlueprintCallable, Category = "Mixed Reality Hand")
	void DebugDraw();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mixed Reality Hand")
	EControllerHand Handedness;

protected:

	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mixed Reality Hand", meta = (AllowPrivateAccess = "true"))
	UTouchPointer *TouchPointer;

	FColor GetColor() const;

};
