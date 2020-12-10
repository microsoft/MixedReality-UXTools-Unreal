// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "UxtMathUtilsFunctionLibrary.generated.h"

/**
 * Library of utility functions for UX Tools.
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API UUxtMathUtilsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Compute minimal rotation between vectors.
	 */
	UFUNCTION(BlueprintPure, Category = "UXTools|Math Utils")
	static FRotator GetRotationBetweenVectors(const FVector& Vector1, const FVector& Vector2);

	/**
	 * Decompose a rotation into swing and twist components.
	 * The twist component describes a rotation around the given twist axis, while the swing contains the remaining rotation.
	 */
	UFUNCTION(BlueprintPure, Category = "UXTools|Math Utils")
	static void SwingTwistDecompose(const FRotator& Rotation, const FVector& TwistAxis, FRotator& Swing, FRotator& Twist);

	/**
	 * Apply rotation about a pivot point to the transform.
	 */
	UFUNCTION(BlueprintPure, Category = "UXTools|Math Utils")
	static FTransform RotateAboutPivotPoint(const FTransform& Transform, const FRotator& Rotation, const FVector& Pivot);

	/**
	 * Function pointer which takes a scene component and returns true if the component should be considered for hierarchy bounds
	 * calculations.
	 */
	typedef bool (*HierarchyBoundsFilter)(const USceneComponent* Component);

	/**
	 * Calculates the composite bounding box and bounding sphere around a component and its children, the output is in
	 * the space of the component. The optional filter component can be used to ignore specific scene components.
	 */
	static FBoxSphereBounds CalculateHierarchyBounds(USceneComponent* Component, HierarchyBoundsFilter Filter = nullptr)
	{
		return CalculateHierarchyBounds(Component, FTransform::Identity, Filter);
	}

	/**
	 * Calculates the composite bounding box and bounding sphere around a component and its children. The optional filter component can be
	 * used to ignore specific scene components.
	 */
	static FBoxSphereBounds CalculateHierarchyBounds(
		USceneComponent* Component, const FTransform& LocalToTarget, HierarchyBoundsFilter Filter = nullptr);

	/**
	 * Calculates the actor bounds for a given space transform
	 */
	static FBox CalculateNestedActorBoundsInGivenSpace(
		const AActor* Actor, const FTransform& WorldToCalcSpace, bool bNonColliding, UPrimitiveComponent* Ignore = nullptr);

	/**
	 * Calculates the actor bounds in local space
	 */
	static FBox CalculateNestedActorBoundsInLocalSpace(const AActor* Actor, bool bNonColliding, UPrimitiveComponent* Ignore = nullptr);
};
