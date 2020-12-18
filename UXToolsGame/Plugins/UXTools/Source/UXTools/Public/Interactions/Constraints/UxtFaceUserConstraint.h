// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"

#include "UxtFaceUserConstraint.generated.h"

/**
 * Component for fixing the rotation of a manipulated object such that
 * it always faces (or faces away from) the user
 *
 * Usage:
 * Attach to actor that the constraint should be applied to.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtFaceUserConstraint : public UUxtTransformConstraint
{
	GENERATED_BODY()
public:
	virtual EUxtTransformMode GetConstraintType() const;
	virtual void ApplyConstraint(FTransform& Transform) const;

public:
	/** Option to use this constraint to face away from the user. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint|Face User")
	bool bFaceAway = false;
};
