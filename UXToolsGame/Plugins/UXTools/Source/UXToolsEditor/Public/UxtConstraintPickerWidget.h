// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "SSCSEditor.h"

#include "Interactions/Constraints/UxtTransformConstraint.h"

#include "UxtConstraintPickerWidget.generated.h"

/**
 * Editor utility widget for adding constraints to actors.
 */
UCLASS(ClassGroup = UXToolsEditor)
class UUxtConstraintPickerWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/** Sets the unique identifier for the tab this EditorUtilityWidget was spawned into. */
	void SetTabID(FName ID);

	/** Set the wiget's owning component. */
	void SetOwner(TWeakObjectPtr<UObject> Owner);

	/** Returns true if we have a valid handle to the actor being modified. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constraint Picker Widget")
	bool HasValidOwner() const;

protected:
	/** Event which triggers when the owner is changed. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Uxt Constraint Picker Widget")
	void OnOwnerChanged();

	/** Get all classes that inherit from UxtTransformConstraint. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constraint Picker Widget")
	TArray<TSubclassOf<UUxtTransformConstraint>> GetConstraintClasses() const;

	/** Get the name of the constraint as a formatted display string. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constraint Picker Widget")
	FString GetConstraintName(TSubclassOf<UUxtTransformConstraint> ConstraintClass);

	/** Get the description for the constraint, this is the comment above the constraint class. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constraint Picker Widget")
	FString GetConstraintDescription(TSubclassOf<UUxtTransformConstraint> ConstraintClass);

	/** Add the selected constraint to the actor / blueprint. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Constraint Picker Widget")
	bool AddConstraint(TSubclassOf<UUxtTransformConstraint> ConstraintClass);

	/** Gets unique identifier for the tab this EditorUtilityWidget was spawned into. */
	UPROPERTY(BlueprintReadOnly, Category = "Uxt Constraint Picker Widget")
	FName TabID;

private:
	TSharedPtr<SSCSEditor> SCSEditor;
};
