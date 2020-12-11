// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"

#include "UxtToggleGroupComponent.generated.h"

class UUxtToggleStateComponent;
class UUxtToggleGroupComponent;

//
// Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUxtToggleGroupSelectionChangedDelegate, UUxtToggleGroupComponent*, ToggleGroup);

/**
 * Component which controls the state of a collection of UUxtToggleStateComponent to behave like a radio group.
 * The component ensures that only one toggle state can be toggled on at a time. Optionally, all states can be
 * toggled off if the SelectedIndex is set to INDEX_NONE.
 */
UCLASS(ClassGroup = "UXTools", meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtToggleGroupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Mutates the selected index, broadcasts events, and performs bounds checking. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Toggle Group")
	void SetSelectedIndex(int32 Index);

	/** Accessor to the selected index. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Toggle Group")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	/** Adds a toggle state to the end of the ToggleStates list. Returns true if the insertion was successful. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Toggle Group")
	bool AddToggleState(UUxtToggleStateComponent* ToggleState);

	/** Adds a toggle state to a specific index within the ToggleStates list. Increments the selection index if the
		toggle state is added before the current selection index. Returns true if the insertion was successful. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Toggle Group")
	bool InsertToggleState(UUxtToggleStateComponent* ToggleState, int32 Index);

	/** Removes a toggle state from the ToggleStates list. Returns true if the removal was successful. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Toggle Group")
	bool RemoveToggleState(UUxtToggleStateComponent* ToggleState);

	/** Removes all toggle states from the ToggleStates list and invalidates the selected index. */
	UFUNCTION(BlueprintCallable, Category = "Uxt Toggle Group")
	void EmptyGroup();

	/** Returns how many toggle states are within the group. */
	UFUNCTION(BlueprintPure, Category = "Uxt Toggle Group")
	int32 GetGroupCount() const { return ToggleStates.Num(); }

	/** Returns the index of the ToggleState in the ToggleStates list, if the ToggleState does not exist returns INDEX_NONE (-1). */
	UFUNCTION(BlueprintPure, Category = "Uxt Toggle Group")
	int32 GetToggleStateIndex(const UUxtToggleStateComponent* ToggleState) const;

	//
	// Events

	/** Event which broadcasts when the toggle group selection changes. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Toggle Group")
	FUxtToggleGroupSelectionChangedDelegate OnGroupSelectionChanged;

protected:
	/** Extracts references from the ToggleReferences and sets the initial selection. */
	virtual void BeginPlay() override;

	/** Delegate for when any toggle state within ToggleStates is toggled.  */
	UFUNCTION()
	virtual void OnToggled(UUxtToggleStateComponent* ToggleState);

	typedef TWeakObjectPtr<UUxtToggleStateComponent> ToggleStateWeak;

	/** A collection of toggle states that act as one toggle group, only one state can be toggle on at a time. */
	TArray<ToggleStateWeak> ToggleStates;

private:
	/** Defragments the list of ToggleStates in case any components were deleted without explicitly being removed. */
	void CompactLostReferences();

	/** Toggles on the selected index and toggles off the other indicies. Also broadcasts a selection change. */
	void ApplySelection();

	/** The currently selected index within the group. A value of INDEX_NONE (-1) means nothing within the group is selected. */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Toggle Group", BlueprintSetter = "SetSelectedIndex", BlueprintGetter = "GetSelectedIndex",
		meta = (ClampMin = "-1"))
	int32 SelectedIndex = INDEX_NONE;

	/** Details panel exposed list of toggle states to initially populate the ToggleStates list with. The component property name is
	   optional, if `None` is specified the first UUxtToggleStateComponent found is used.*/
	UPROPERTY(EditAnywhere, Category = "Uxt Toggle Group", meta = (AllowedClasses = "UUxtToggleStateComponent"))
	TArray<FComponentReference> ToggleReferences;
};
