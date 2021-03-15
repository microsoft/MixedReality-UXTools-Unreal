// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Constraints/UxtTransformConstraint.h"

#include "UxtManipulatorComponent.generated.h"

/**
 * Manages constraints that can be applied by child components.
 */
UCLASS(Abstract, Blueprintable, Category = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtManipulatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Get if the component is automatically detecting constraints. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Manipulator")
	bool GetAutoDetectConstraints() const;

	/** Set if the component should automatically detect constraints. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Manipulator")
	void SetAutoDetectConstraints(bool bNewAutoDetectConstraints);

	/** Get the list of currently selected constraints. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Manipulator")
	const TArray<FComponentReference>& GetSelectedConstraints() const;

	/** Add a constraint to be applied when bAutoDetectConstraints is not set. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator")
	void AddConstraint(const FComponentReference& NewConstraint);

	/** Remove a constraint from being applied when bAutoDetectConstraints is not set. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator")
	void RemoveConstraint(const FComponentReference& NewConstraint);

	//
	// Implicit scale constraint's public API

	UFUNCTION(BlueprintGetter, Category = "Uxt Manipulator")
	bool GetRelativeToInitialScale() const { return bRelativeToInitialScale; }

	UFUNCTION(BlueprintSetter, Category = "Uxt Manipulator")
	void SetRelativeToInitialScale(const bool Value);

	UFUNCTION(BlueprintGetter, Category = "Uxt Manipulator")
	float GetMinScale() const { return MinScale; }

	UFUNCTION(BlueprintSetter, Category = "Uxt Manipulator")
	void SetMinScale(const float Value);

	UFUNCTION(BlueprintGetter, Category = "Uxt Manipulator")
	float GetMaxScale() const { return MaxScale; }

	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator")
	void SetMaxScale(const float Value);

protected:
	//
	// UActorComponent interface
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Initialize the constraints with a target component to use for a reference transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator")
	void InitializeConstraints(USceneComponent* NewTargetComponent);

	/** Apply the constraints to the transform. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator")
	void ApplyConstraints(FTransform& Transform, EUxtTransformMode TransformMode, bool bIsOneHanded, bool bIsNear) const;

	/** Get notified of another component starting a new manipulation. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Manipulator")
	virtual void OnExternalManipulationStarted() PURE_VIRTUAL(UUxtManipulatorComponent::OnExternalManipulationStarted);

	/** Notifies other manipulator components by calling @ref OnExternalManipulationStarted on them. */
	void NotifyManipulationStarted();

private:
	/** Get a list of constraints that should be applied. */
	TArray<UUxtTransformConstraint*> GetConstraints() const;

	/** Update the list registed constraints to be applied. */
	void UpdateActiveConstraints();

	/** Converts @ref MinScale and @ref MaxScale between relative/absolute, based on the value of @ref bRelativeToInitialScale. */
	void ConvertMinMaxScaleValues();

	/** Returns a vector with the minimum scale allowed by configuration. */
	FVector GetMinScaleVec() const;
	/** Returns a vector with the maximum scale allowed by configuration. */
	FVector GetMaxScaleVec() const;

	/** If set, all constraints present on the actor will be applied, otherwise only selected constraints will be applied. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Manipulator", BlueprintGetter = GetAutoDetectConstraints, BlueprintSetter = SetAutoDetectConstraints)
	bool bAutoDetectConstraints = true;

	/** The list of constraints to be applied if bAutoDetectConstraints is false. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Manipulator", BlueprintGetter = GetSelectedConstraints,
		meta = (/*UseComponentPicker, AllowedClasses = "UxtTransformConstraint",*/ EditCondition = "!bAutoDetectConstraints"))
	TArray<FComponentReference> SelectedConstraints;

	/** Whether the min/max scale values should be relative to the initial scale or absolute. */
	UPROPERTY(
		EditAnywhere, BlueprintGetter = GetRelativeToInitialScale, BlueprintSetter = SetRelativeToInitialScale,
		Category = "Uxt Manipulator")
	bool bRelativeToInitialScale = true;

	/** Minimum scale allowed. Will be used as relative or absolute depending on the value of @ref bRelativeToInitialScale. */
	UPROPERTY(EditAnywhere, Category = "Uxt Manipulator", BlueprintGetter = GetMinScale, BlueprintSetter = SetMinScale)
	float MinScale = 0.2f;

	/** Maximum scale allowed. Will be used as relative or absolute depending on the value of @ref bRelativeToInitialScale. */
	UPROPERTY(EditAnywhere, Category = "Uxt Manipulator", BlueprintGetter = GetMaxScale, BlueprintSetter = SetMaxScale)
	float MaxScale = 2.0f;

	/** The list constraints currently being applied. */
	TArray<UUxtTransformConstraint*> ActiveConstraints;

	/** The component to use for a reference transform when initializing constraints. */
	USceneComponent* TargetComponent = nullptr;

	FVector InitialScale;
};
