// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"

#include "UxtFixedRotationToUserConstraint.generated.h"

/**
 * Component for fixing the rotation of a manipulated object relative to the user
 *
 * Usage:
 * Attach to actor that the constraint should be applied to.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFixedRotationToUserConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual void Initialize(const FTransform& WorldPose) override;
	virtual EUxtTransformMode GetConstraintType() const override;
	virtual void ApplyConstraint(FTransform& Transform) const override;

public:
	/** Should roll be excluded from locking to the users orientation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint|Fixed Rotation To User")
	bool bExcludeRoll = true;

private:
	FQuat StartObjectRotationCameraSpace;
};
