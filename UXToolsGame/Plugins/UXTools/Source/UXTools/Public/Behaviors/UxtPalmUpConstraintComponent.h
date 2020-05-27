// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Behaviors/UxtHandConstraintComponent.h"
#include "UxtPalmUpConstraintComponent.generated.h"


/**
 * Hand constraint component that becomes active if the hand is facing the player camera.
 * 
 * The palm must be facing the camera for the constraint to be active.
 * Optionally the hand can also be rejected if it isn't flat.
 */
UCLASS(Blueprintable, ClassGroup = UXTools, meta=(BlueprintSpawnableComponent))
class UXTOOLS_API UUxtPalmUpConstraintComponent : public UUxtHandConstraintComponent
{
	GENERATED_BODY()

public:

	virtual bool IsHandUsableForConstraint(EControllerHand NewHand) const override;

public:

	/**
	 * Maximum allowed angle between the negative palm normal and view vector.
	 * If the angle exceeds the limit the hand is not used.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PalmUp Constraint", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPalmAngle = 80.0f;

	/**
	 * If true then the hand needs to be flat to be accepted.
	 * The triangle between index, ring finger, and palm needs to be aligned with the palm within MaxFlatHandAngle.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PalmUp Constraint")
	bool bRequireFlatHand = false;

	/** Maximum allowed angle between palm and index/ring finger/palm triangle to be considered a flat hand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PalmUp Constraint", meta = (EditCondition = "bRequireFlatHand", ClampMin = "0.0", ClampMax = "90.0"))
	float MaxFlatHandAngle = 45.0f;

};
