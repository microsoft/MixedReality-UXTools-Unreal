// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBaseObjectCollection.h"

#include "GameFramework/Actor.h"

namespace
{
	struct ComparisonPredicate
	{
		FORCEINLINE bool operator()(const AActor& A, const AActor& B) const { return ComparisonFunctor.Execute(&A, &B); }

		ComparisonPredicate(FUxtSortScrollingObjectCollectionDelegate Functor) : ComparisonFunctor(Functor) {}

		FUxtSortScrollingObjectCollectionDelegate ComparisonFunctor;
	};
} // namespace

/**
 *
 */
const TArray<AActor*>& UUxtBaseObjectCollection::CollectAttachedActors()
{
	CollectionObjectInterfaceComponents.Reset(CollectionObjectInterfaceComponents.Num());

	const AActor* Owner = GetOwner();
	if (Owner)
	{
		const bool bResetArray = false;
		Owner->GetAttachedActors(AttachedActors, bResetArray);

		// Give the user an opportunity to sort the array of attached actors
		if (SortCollection.IsBound())
		{
			AttachedActors.Sort(ComparisonPredicate(SortCollection));
		}

		for (AActor* Actor : AttachedActors)
		{
			TSet<UActorComponent*> Components = Actor->GetComponents();
			for (UActorComponent* Comp : Components)
			{
				if (Comp->Implements<UUxtCollectionObject>())
				{
					CollectionObjectInterfaceComponents.Add(Comp);
				}
			}
		}
	}

	return AttachedActors;
}

/**
 *
 */
