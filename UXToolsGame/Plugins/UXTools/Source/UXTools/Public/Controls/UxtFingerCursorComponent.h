// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtRingCursorComponent.h"

#include "UxtFingerCursorComponent.generated.h"

class UUxtNearPointerComponent;

/**
 * When added to an actor with a near pointer, this component displays a ring cursor oriented towards the current poke target and
 * scaled according to the distance.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFingerCursorComponent : public UUxtRingCursorComponent
{
	GENERATED_BODY()

public:
	UUxtFingerCursorComponent();

	/** Cursor scale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Finger Cursor")
	float CursorScale = 1.0f;

	/** Show the finger cursor on grab targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Finger Cursor")
	bool bShowOnGrabTargets = false;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Distance at which the cursor starts to align with pokable surfaces. */
	UPROPERTY(EditAnywhere, Category = "Uxt Finger Cursor")
	float AlignWithSurfaceDistance = 10.0f;

private:
	/** Dynamic instance of the material. */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* FingerMaterialInstance;

	/** Near pointer in use. */
	TWeakObjectPtr<UUxtNearPointerComponent> HandPointerWeak;

	/** Scaler applied to the Proximity Distance to fade the cursor in when enabled. */
	float CursorFadeScaler;
};
