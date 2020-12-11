// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"

#include "UxtMinMaxScaleConstraint.generated.h"

/**
 * Constraint that limits the min/max scale.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtMinMaxScaleConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual EUxtTransformMode GetConstraintType() const;

	virtual void ApplyConstraint(FTransform& Transform) const;

	virtual void Initialize(const FTransform& WorldPose);

	/** Minimum scale allowed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint | Min Max Scale", meta = (Bitmask, BitmaskEnum = EUxtAxisFlags))
	float MinScaleFactor = 0.0f;

	/** Maximum scale allowed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint | Min Max Scale", meta = (Bitmask, BitmaskEnum = EUxtAxisFlags))
	float MaxScaleFactor = 0.0f;

	/** Whether the min/max values should be relative to the initial transform or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint | Min Max Scale")
	bool bRelativeToInitialTransform = false;

private:
	/** By using those vectors, the scales to apply aren't recalculated on each apply call. */
	FVector MinScaleVec;
	FVector MaxScaleVec;
};
