// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/SceneComponent.h"
#include "Controls/UxtCollectionObject.h"

#include "UxtBaseObjectCollection.generated.h"

//
// Delegates
DECLARE_DYNAMIC_DELEGATE_RetVal_TwoParams(bool, FUxtSortScrollingObjectCollectionDelegate, const AActor*, LHS, const AActor*, RHS);

/**
 * Base scene component class for object collections
 */
UCLASS(ClassGroup = ("UXTools - Experimental"))
class UXTOOLS_API UUxtBaseObjectCollection : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Set the the callback function to be used by the sort to compare actor pairs.
	 *	In order to see results of sorting with the editor it is necessary to enable run in editor in the functions details panel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Uxt Base Object Collection - Experimental", meta = (AutoCreateRefTerm = "Callback"))
	void SetSortCallback(const FUxtSortScrollingObjectCollectionDelegate& Callback) { SortCollection = Callback; }

	//
	//  Events
	/*  Event raised to allow the blueprint to override the default sorting method. */
	UPROPERTY()
	FUxtSortScrollingObjectCollectionDelegate SortCollection;

	/** Helper function to allow Blueprint class to easily compare a pair of text objects within sort. */
	UFUNCTION(BlueprintPure, Category = "UXTools|Base Object Collection - Experimental")
	static bool CompareText(const FText& LHS, const FText& RHS) { return LHS.ToString() > RHS.ToString(); }

	/** Helper function to allow Blueprint class to easily compare a pair of string objects within sort. */
	UFUNCTION(BlueprintPure, Category = "UXTools|Base Object Collection - Experimental")
	static bool CompareString(const FString& LHS, const FString& RHS) { return LHS > RHS; }

protected:
	/** Get the array of attached actors. */
	const TArray<AActor*>& GetAttachedActors() const;

	/** Collect array of actors that are currently attached to the actor that this component is a part of. */
	const TArray<AActor*>& CollectAttachedActors();

private:
	/** Array of actors attached to this collection. */
	TArray<AActor*> AttachedActors;

	/** Up to date list of components in the collection that implement the IUxtCollectionObject interface. */
	TArray<UActorComponent*> CollectionObjectInterfaceComponents;
};

inline const TArray<AActor*>& UUxtBaseObjectCollection::GetAttachedActors() const
{
	return AttachedActors;
}
