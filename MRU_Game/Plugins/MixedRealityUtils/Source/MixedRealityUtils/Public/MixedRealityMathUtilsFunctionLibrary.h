// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MixedRealityMathUtilsFunctionLibrary.generated.h"

/**
 * Library of utility functions for Mixed Reality.
 */
UCLASS()
class MIXEDREALITYUTILS_API UMixedRealityMathUtilsFunctionLibrary : public UBlueprintFunctionLibrary
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
