#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TouchPointerTarget.h"
#include "TestTouchPointerTarget.generated.h"

/**
 * Target for touch pointer tests that counts touch events.
 */
UCLASS()
class MIXEDREALITYUTILSTESTS_API UTestTouchPointerTarget : public USceneComponent, public ITouchPointerTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// ITouchPointerTarget interface

	virtual void TouchStarted_Implementation(UTouchPointer* Pointer) override;
	virtual void TouchEnded_Implementation(UTouchPointer* Pointer) override;

	virtual bool GetClosestPointOnSurface_Implementation(const FVector& Point, FVector& OutPointOnSurface) override;

	int TouchStartedCount;
	int TouchEndedCount;

};
