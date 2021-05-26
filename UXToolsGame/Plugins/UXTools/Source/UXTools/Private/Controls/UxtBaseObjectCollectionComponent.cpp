// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtBaseObjectCollectionComponent.h"

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

void UUxtBaseObjectCollectionComponent::SetCollectChildActors(bool CollectChildActors)
{
	bCollectChildActors = CollectChildActors;

	RefreshCollection();
}

void UUxtBaseObjectCollectionComponent::RefreshCollection()
{
}

const TArray<AActor*>& UUxtBaseObjectCollectionComponent::CollectAttachedActors()
{
	const AActor* Owner = GetOwner();
	if (Owner)
	{
		const bool bResetArray = true;
		Owner->GetAttachedActors(AttachedActors, bResetArray);

		if (!bCollectChildActors)
		{
			// Remove attached actors which came from child actor components.
			for (int Index = 0; Index < AttachedActors.Num();)
			{
				if (AttachedActors[Index]->IsChildActor())
				{
					AttachedActors.RemoveAt(Index);
				}
				else
				{
					++Index;
				}
			}
		}

		// Give the user an opportunity to sort the array of attached actors
		if (SortCollection.IsBound())
		{
			AttachedActors.Sort(ComparisonPredicate(SortCollection));
		}
	}

	return AttachedActors;
}
