// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtToggleGroupComponent.h"

#include "UXTools.h"

#include "Components/ChildActorComponent.h"
#include "Controls/UxtToggleStateComponent.h"

#include <GameFramework/Actor.h>

void UUxtToggleGroupComponent::SetSelectedIndex(int32 Index)
{
	SelectedIndex = Index;
	ApplySelection();
}

bool UUxtToggleGroupComponent::AddToggleState(UUxtToggleStateComponent* ToggleState)
{
	return InsertToggleState(ToggleState, ToggleStates.Num());
}

bool UUxtToggleGroupComponent::InsertToggleState(UUxtToggleStateComponent* ToggleState, int32 Index)
{
	if ((ToggleState != nullptr) && !ToggleStates.Contains(ToggleState))
	{
		ToggleStates.Insert(ToggleState, Index);
		ToggleState->OnToggled.AddDynamic(this, &UUxtToggleGroupComponent::OnToggled);

		// If the insertion happened before the currently selected index, maintain the selected index by incrementing it.
		if (Index <= SelectedIndex)
		{
			SetSelectedIndex(SelectedIndex + 1);
		}

		return true;
	}

	return false;
}

bool UUxtToggleGroupComponent::RemoveToggleState(UUxtToggleStateComponent* ToggleState)
{
	if (ToggleState != nullptr)
	{
		int32 Index = ToggleStates.Find(ToggleState);

		if (Index != INDEX_NONE)
		{
			ToggleState->OnToggled.RemoveDynamic(this, &UUxtToggleGroupComponent::OnToggled);
			ToggleStates.RemoveAt(Index);

			if (Index == SelectedIndex)
			{
				// If the selected index is removed, invalidate the selection.
				SetSelectedIndex(INDEX_NONE);
			}
			else if (Index <= SelectedIndex)
			{
				// If the removal is before the currently selected index, maintain the selected index by decrementing it.
				SetSelectedIndex(SelectedIndex - 1);
			}

			return true;
		}
	}

	return false;
}

void UUxtToggleGroupComponent::EmptyGroup()
{
	for (int32 Index = ToggleStates.Num() - 1; Index >= 0; --Index)
	{
		if (ToggleStates[Index].IsValid())
		{
			RemoveToggleState(ToggleStates[Index].Get());
		}
		else
		{
			ToggleStates.RemoveAt(Index);
		}
	}

	SetSelectedIndex(INDEX_NONE);
}

int32 UUxtToggleGroupComponent::GetToggleStateIndex(const UUxtToggleStateComponent* ToggleState) const
{
	return ToggleStates.IndexOfByKey(ToggleState);
}

void UUxtToggleGroupComponent::BeginPlay()
{
	Super::BeginPlay();

	// Acquire all of the toggle states from the references.
	for (FComponentReference& ComponentReference : ToggleReferences)
	{
		AActor* ActorToSearch = GetOwner();
		UUxtToggleStateComponent* ToggleState = Cast<UUxtToggleStateComponent>(ComponentReference.GetComponent(ActorToSearch));

		// If the ComponentReference didn't find a UUxtToggleStateComponent via the actor and component name.
		if (ToggleState == nullptr)
		{
			// Check if the ComponentReference points to a child actor.
			if (UChildActorComponent* ChildActorComponent = Cast<UChildActorComponent>(ComponentReference.GetComponent(ActorToSearch)))
			{
				// If this is a child actor, try to find any UUxtToggleStateComponent on the child.
				AActor* ChildActor = ChildActorComponent->GetChildActor();

				if (ChildActor != nullptr)
				{
					ActorToSearch = ChildActor;
					ToggleState =
						Cast<UUxtToggleStateComponent>(ActorToSearch->GetComponentByClass(UUxtToggleStateComponent::StaticClass()));
				}
			}
			else if (ComponentReference.OtherActor != nullptr)
			{
				// If an actor name was specified, but not a valid component name try to find any UUxtToggleStateComponent on the actor.
				ActorToSearch = ComponentReference.OtherActor;
				ToggleState = Cast<UUxtToggleStateComponent>(ActorToSearch->GetComponentByClass(UUxtToggleStateComponent::StaticClass()));
			}
		}

		if (ToggleState != nullptr)
		{
			ToggleStates.Add(ToggleState);
			ToggleState->OnToggled.AddDynamic(this, &UUxtToggleGroupComponent::OnToggled);
		}
		else
		{
			UE_LOG(
				UXTools, Error,
				TEXT("The UUxtToggleGroupComponent '%s' contains a reference to an actor named '%s' that does not contain a "
					 "UUxtToggleGroupComponent."),
				*GetOwner()->GetName(), *ActorToSearch->GetName());
		}
	}

	ApplySelection();
}

void UUxtToggleGroupComponent::OnToggled(UUxtToggleStateComponent* ToggleState)
{
	int32 Index = ToggleStates.Find(ToggleState);

	if (Index != INDEX_NONE)
	{
		SetSelectedIndex(Index);
	}
}

void UUxtToggleGroupComponent::CompactLostReferences()
{
	for (int32 Index = 0; Index < ToggleStates.Num();)
	{
		if (ToggleStates[Index].IsValid())
		{
			++Index;
		}
		else
		{
			ToggleStates.RemoveAt(Index);
		}
	}
}

void UUxtToggleGroupComponent::ApplySelection()
{
	CompactLostReferences();

	// Clamp the index in case any toggle states were removed.
	SelectedIndex = FMath::Clamp(SelectedIndex, static_cast<int32>(INDEX_NONE), ToggleStates.Num() - 1);

	// Toggle the selected index on, and un-selected indicies off.
	for (int32 Index = 0; Index < ToggleStates.Num(); ++Index)
	{
		ToggleStateWeak& ToggleState = ToggleStates[Index];
		ToggleState->OnToggled.RemoveDynamic(this, &UUxtToggleGroupComponent::OnToggled);
		ToggleState->SetIsChecked(SelectedIndex == Index);
		ToggleState->OnToggled.AddDynamic(this, &UUxtToggleGroupComponent::OnToggled);
	}

	OnGroupSelectionChanged.Broadcast(this);
}
