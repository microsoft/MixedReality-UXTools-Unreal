// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Utils/UxtMathUtilsFunctionLibrary.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

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

FBox UUxtMathUtilsFunctionLibrary::CalculateNestedActorBoundsInGivenSpace(
	const AActor* Actor, const FTransform& WorldToCalcSpace, bool bNonColliding, UPrimitiveComponent* Ignore)
{
	FBox Box(ForceInit);

	for (const UActorComponent* ActorComponent : Actor->GetComponents())
	{
		if (!ActorComponent->IsRegistered())
		{
			continue;
		}

		if (const UPrimitiveComponent* PrimitiveComponent = Cast<const UPrimitiveComponent>(ActorComponent))
		{
			if (PrimitiveComponent == Ignore)
			{
				continue;
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

		if (const UChildActorComponent* ChildActor = Cast<const UChildActorComponent>(ActorComponent))
		{
			if (const AActor* NestedActor = ChildActor->GetChildActor())
			{
				Box += CalculateNestedActorBoundsInGivenSpace(NestedActor, WorldToCalcSpace, bNonColliding);
			}
		}
	}

	return Box;
}

FBox UUxtMathUtilsFunctionLibrary::CalculateNestedActorBoundsInLocalSpace(
	const AActor* Actor, bool bNonColliding, UPrimitiveComponent* Ignore)
{
	const FTransform& ActorToWorld = Actor->GetTransform();
	const FTransform WorldToActor = ActorToWorld.Inverse();

	return UUxtMathUtilsFunctionLibrary::CalculateNestedActorBoundsInGivenSpace(Actor, WorldToActor, true, Ignore);
}
