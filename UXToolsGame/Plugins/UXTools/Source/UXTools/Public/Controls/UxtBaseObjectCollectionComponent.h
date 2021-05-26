// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UxtUIElementComponent.h"

#include "Components/SceneComponent.h"
#include "Controls/UxtCollectionObject.h"

#include "UxtBaseObjectCollectionComponent.generated.h"

//
// Delegates
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FUxtSortScrollingObjectCollectionDelegate, const AActor*, LHS, const AActor*, RHS);

/**
 * Base scene component class for object collections
 */
UCLASS(ClassGroup = ("UXTools"))
class UXTOOLS_API UUxtBaseObjectCollectionComponent : public UUxtUIElementComponent
{
	GENERATED_BODY()

public:
	/** Set the the callback function to be used by the sort to compare actor pairs.
	 *	In order to see results of sorting with the editor it is necessary to enable run in editor in the functions details panel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Base Object Collection", meta = (AutoCreateRefTerm = "Callback"))
	void SetSortCallback(const FUxtSortScrollingObjectCollectionDelegate& Callback) { SortCollection = Callback; }

	/** Gets if the collection should collect child actors. */
	UFUNCTION(BlueprintGetter, Category = "Uxt Base Object Collection")
	bool GetCollectChildActors() const { return bCollectChildActors; }

	/** Sets if the collection should collect child actors. */
	UFUNCTION(BlueprintSetter, Category = "Uxt Base Object Collection")
	void SetCollectChildActors(bool CollectChildActors);

	//
	//  Events
	/*  Event raised to allow the blueprint to override the default sorting method. */
	UPROPERTY()
	FUxtSortScrollingObjectCollectionDelegate SortCollection;

	/** Helper function to allow Blueprint class to easily compare a pair of text objects within sort. */
	UFUNCTION(BlueprintPure, Category = "Uxt Base Object Collection")
	static bool CompareText(const FText& LHS, const FText& RHS) { return LHS.ToString() < RHS.ToString(); }

	/** Helper function to allow Blueprint class to easily compare a pair of string objects within sort. */
	UFUNCTION(BlueprintPure, Category = "Uxt Base Object Collection")
	static bool CompareString(const FString& LHS, const FString& RHS) { return LHS < RHS; }

protected:
	/** Called to update the collection based on the current properties. */
	virtual void RefreshCollection();

	/** Get the array of attached actors. */
	const TArray<AActor*>& GetAttachedActors() const { return AttachedActors; }

	/** Collect array of actors that are currently attached to the actor that this component is a part of. */
	const TArray<AActor*>& CollectAttachedActors();

private:
	/** Should child actors (which come from child actor components) be considered as attached actors? */
	UPROPERTY(
		EditAnywhere, Category = "Uxt Base Object Collection", BlueprintGetter = "GetCollectChildActors",
		BlueprintSetter = "SetCollectChildActors", AdvancedDisplay)
	bool bCollectChildActors = false;

	/** Array of actors attached to this collection. */
	TArray<AActor*> AttachedActors;
};
