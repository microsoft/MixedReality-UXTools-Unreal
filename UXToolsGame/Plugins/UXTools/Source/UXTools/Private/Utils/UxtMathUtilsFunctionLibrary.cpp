// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Utils/UxtMathUtilsFunctionLibrary.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Containers/ArrayView.h"
#include "GameFramework/Actor.h"

namespace
{
	void ExpandBoxIfAppropriateComponent(
		FBox& Box, const USceneComponent* const Component, const FTransform& WorldToCalcSpace, bool bNonColliding,
		TArrayView<const UPrimitiveComponent* const> Ignore = {})
	{
		if (!Component->IsRegistered())
		{
			return;
		}
		const UPrimitiveComponent* PrimitiveComponent = Cast<const UPrimitiveComponent>(Component);
		if (!PrimitiveComponent || Ignore.Contains(PrimitiveComponent))
		{
			return;
		}

		// Only use collidable components to find collision bounding box.
		if (bNonColliding || PrimitiveComponent->IsCollisionEnabled())
		{
			const FTransform& ComponentToWorld = PrimitiveComponent->GetComponentTransform();
			const FTransform ComponentToCalcSpace = ComponentToWorld * WorldToCalcSpace;

			const FBoxSphereBounds ComponentBoundsCalcSpace = PrimitiveComponent->CalcBounds(ComponentToCalcSpace);
			const FBox ComponentBox = ComponentBoundsCalcSpace.GetBox();
			Box += ComponentBox;
		}
	}

} // namespace

FRotator UUxtMathUtilsFunctionLibrary::GetRotationBetweenVectors(const FVector& Vector1, const FVector& Vector2)
{
	return FQuat::FindBetween(Vector1, Vector2).Rotator();
}

void UUxtMathUtilsFunctionLibrary::SwingTwistDecompose(const FRotator& Rotation, const FVector& TwistAxis, FRotator& Swing, FRotator& Twist)
{
	FQuat qSwing, qTwist;
	FQuat(Rotation).ToSwingTwist(TwistAxis, qSwing, qTwist);
	Swing = qSwing.Rotator();
	Twist = qTwist.Rotator();
}

FTransform UUxtMathUtilsFunctionLibrary::RotateAboutPivotPoint(const FTransform& Transform, const FRotator& Rotation, const FVector& Pivot)
{
	FTransform result = Transform;
	result.SetLocation(result.GetLocation() - Pivot);
	result *= FTransform(Rotation);
	result.SetLocation(result.GetLocation() + Pivot);
	return result;
}

FBoxSphereBounds UUxtMathUtilsFunctionLibrary::CalculateHierarchyBounds(
	USceneComponent* Component, const FTransform& LocalToTarget, HierarchyBoundsFilter Filter)
{
	FBoxSphereBounds Bounds =
		(Filter != nullptr && Filter(Component)) ? Component->CalcBounds(LocalToTarget) : FBoxSphereBounds(EForceInit::ForceInit);
	for (USceneComponent* Child : Component->GetAttachChildren())
	{
		FTransform ChildLocalToParent = Child->GetRelativeTransform() * LocalToTarget;
		Bounds = Bounds + CalculateHierarchyBounds(Child, ChildLocalToParent, Filter);
	}
	return Bounds;
}

FBox UUxtMathUtilsFunctionLibrary::CalculateNestedBoundsInGivenSpace(
	const USceneComponent* const Root, const FTransform& WorldToCalcSpace, bool bNonColliding,
	TArrayView<const UPrimitiveComponent* const> Ignore)
{
	FBox Box(ForceInit);

	TArray<USceneComponent*> RelevantComponents;
	ExpandBoxIfAppropriateComponent(Box, Root, WorldToCalcSpace, bNonColliding, Ignore);
	Root->GetChildrenComponents(true, RelevantComponents);
	for (const USceneComponent* ChildComponent : RelevantComponents)
	{
		ExpandBoxIfAppropriateComponent(Box, ChildComponent, WorldToCalcSpace, bNonColliding, Ignore);
	}

	return Box;
}