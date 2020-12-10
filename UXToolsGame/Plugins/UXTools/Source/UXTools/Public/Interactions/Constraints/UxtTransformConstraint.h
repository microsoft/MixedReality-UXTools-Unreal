// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Interactions/UxtInteractionMode.h"
#include "Interactions/UxtManipulationFlags.h"

#include "UxtTransformConstraint.generated.h"

/**
 * Base class for all constraints
 *
 * Usage:
 * Derive from this component and implement ApplyConstraint and GetConstraintType.
 * Custom constraints will automatically be picked up by a UxtConstraintManager on the same actor
 */
UCLASS(Abstract, Blueprintable, ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTransformConstraint : public UActorComponent
{
	GENERATED_BODY()
public:
	/** Enabled manipulation modes. */
	virtual EUxtTransformMode GetConstraintType() const PURE_VIRTUAL(, return EUxtTransformMode::Translation;);

	/** Applies constraints to transforms during manipulation */
	virtual void ApplyConstraint(FTransform& Transform) const PURE_VIRTUAL(, );

	/** Intended to be called on manipulation started */
	virtual void Initialize(const FTransform& WorldPose);

public:
	/** The component to transform, will default to the root scene component if not specified */
	UPROPERTY(EditAnywhere, Category = "Uxt Constraint", AdvancedDisplay)
	FComponentReference TargetComponent;

	/** Whether this constraint applies to one hand manipulation, two hand manipulation or both. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint", meta = (Bitmask, BitmaskEnum = EUxtGrabMode))
	int32 HandType = static_cast<int32>(EUxtGrabMode::OneHanded | EUxtGrabMode::TwoHanded);

	/** Whether this constraint applies to near manipulation, far manipulation or both. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Uxt Constraint", meta = (Bitmask, BitmaskEnum = EUxtInteractionMode))
	int32 InteractionMode = static_cast<int32>(EUxtInteractionMode::Near | EUxtInteractionMode::Far);

protected:
	FTransform WorldPoseOnManipulationStart;
};
