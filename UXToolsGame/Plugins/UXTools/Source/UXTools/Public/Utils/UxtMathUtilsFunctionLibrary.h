// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UxtMathUtilsFunctionLibrary.generated.h"

/**
 * Library of utility functions for UX Tools.
 */
UCLASS()
class UXTOOLS_API UUxtMathUtilsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Compute minimal rotation between vectors.
	 */
	UFUNCTION(BlueprintPure, Category = "MathUtils")
	static FRotator GetRotationBetweenVectors(const FVector &Vector1, const FVector &Vector2);

	/**
	 * Decompose a rotation into swing and twist components.
	 * The twist component describes a rotation around the given twist axis, while the swing contains the remaining rotation.
	 */
	UFUNCTION(BlueprintPure, Category = "MathUtils")
	static void SwingTwistDecompose(const FRotator &Rotation, const FVector &TwistAxis, FRotator &Swing, FRotator &Twist);

	/**
	 * Apply rotation about a pivot point to the transform.
	 */
	UFUNCTION(BlueprintPure, Category = "MathUtils")
	static FTransform RotateAboutPivotPoint(const FTransform &Transform, const FRotator &Rotation, const FVector &Pivot);

};

