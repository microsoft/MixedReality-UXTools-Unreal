// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Utils/UxtMathUtilsFunctionLibrary.h"
#include "Components/SceneComponent.h"

FRotator UUxtMathUtilsFunctionLibrary::GetRotationBetweenVectors(const FVector &Vector1, const FVector &Vector2)
{
	return FQuat::FindBetween(Vector1, Vector2).Rotator();
}

void UUxtMathUtilsFunctionLibrary::SwingTwistDecompose(const FRotator &Rotation, const FVector &TwistAxis, FRotator &Swing, FRotator &Twist)
{
	FQuat qSwing, qTwist;
	FQuat(Rotation).ToSwingTwist(TwistAxis, qSwing, qTwist);
	Swing = qSwing.Rotator();
	Twist = qTwist.Rotator();
}

FTransform UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(const FTransform &Transform, const FRotator &Rotation, const FVector &Pivot)
{
	FTransform result = Transform;
	result.SetLocation(result.GetLocation() - Pivot);
	result *= FTransform(Rotation);
	result.SetLocation(result.GetLocation() + Pivot);
	return result;
}

FBoxSphereBounds UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(USceneComponent* Component, const FTransform& LocalToTarget, HierarchyBoundsFilter Filter)
{
	FBoxSphereBounds Bounds = (Filter != nullptr && Filter(Component)) ? Component->CalcBounds(LocalToTarget) : FBoxSphereBounds(EForceInit::ForceInit);
	for (USceneComponent* Child : Component->GetAttachChildren())
	{
		FTransform ChildLocalToParent = Child->GetRelativeTransform() * LocalToTarget;
		Bounds = Bounds + CalculateHierarchyBounds(Child, ChildLocalToParent, Filter);
	}
	return Bounds;
}

