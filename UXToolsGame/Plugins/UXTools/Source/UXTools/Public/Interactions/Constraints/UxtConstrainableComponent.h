// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtTransformConstraint.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"

#include "UxtConstrainableComponent.generated.h"

/**
 * Manages constraints that can be applied by child components.
 */
UCLASS(Abstract, Blueprintable, Category = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtConstrainableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Get if the component is automatically detecting constraints. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Constrainable")
	bool GetAutoDetectConstraints() const;

	/** Set if the component should automatically detect constraints. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Constrainable")
	void SetAutoDetectConstraints(bool bNewAutoDetectConstraints);

	/** Get the list of currently selected constraints. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Constrainable")
	const TArray<FComponentReference>& GetSelectedConstraints() const;

	/** Add a constraint to be applied when bAutoDetectConstraints is not set. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constrainable")
	void AddConstraint(const FComponentReference& NewConstraint);

	/** Remove a constraint from being applied when bAutoDetectConstraints is not set. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constrainable")
	void RemoveConstraint(const FComponentReference& NewConstraint);

protected:
	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Initialize the constraints with a target component to use for a reference transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constrainable")
	void InitializeConstraints(USceneComponent* NewTargetComponent);

	/** Apply the constraints to the transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constrainable")
	void ApplyConstraints(FTransform& Transform, EUxtTransformMode TransformMode, bool bIsOneHanded, bool bIsNear) const;

private:
	/** Get a list of constraints that should be applied. */
	TArray<UUxtTransformConstraint*> GetConstraints() const;

	/** Update the list registed constraints to be applied. */
	void UpdateActiveConstraints();

	/** If set, all constraints present on the actor will be applied, otherwise only selected constraints will be applied. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Constrainable", BlueprintGetter = GetAutoDetectConstraints,
		BlueprintSetter = SetAutoDetectConstraints)
	bool bAutoDetectConstraints = true;

	/** The list of constraints to be applied if bAutoDetectConstraints is false. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Constrainable", BlueprintGetter = GetSelectedConstraints,
		meta = (/*UseComponentPicker, AllowedClasses = "UxtTransformConstraint",*/ EditCondition = "!bAutoDetectConstraints"))
	TArray<FComponentReference> SelectedConstraints;

	/** The list constraints currently being applied. */
	TArray<UUxtTransformConstraint*> ActiveConstraints;

	/** The component to use for a reference transform when initializing constraints. */
	USceneComponent* TargetComponent = nullptr;
};
