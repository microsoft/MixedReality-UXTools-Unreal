// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtUIElement.h"

#include "GameFramework/Actor.h"

void UUxtUIElement::BeginPlay()
{
	Super::BeginPlay();

	if (!bIsElementActive)
	{
		UpdateVisibility();
	}
}

bool UUxtUIElement::IsElementActiveSelf() const
{
	return bIsElementActive;
}

bool UUxtUIElement::IsElementActiveInHierarchy() const
{
	return !GetOwner()->IsHidden();
}

void UUxtUIElement::SetElementActive(bool bNewActive)
{
	if (bNewActive != bIsElementActive)
	{
		bIsElementActive = bNewActive;

		UpdateVisibility(IsParentActive());
	}
}

void UUxtUIElement::RefreshElement()
{
	UpdateVisibility(IsParentActive());
}

bool UUxtUIElement::IsParentActive() const
{
	const AActor* Parent = GetOwner()->GetAttachParentActor();

	if (Parent)
	{
		const UUxtUIElement* ParentUIElement = Parent->FindComponentByClass<UUxtUIElement>();

		if (ParentUIElement)
		{
			return ParentUIElement->IsElementActiveInHierarchy();
		}
	}

	return true;
}

void UUxtUIElement::UpdateVisibility(bool bParentIsActive)
{
	AActor* Actor = GetOwner();
	const bool bActive = bParentIsActive && bIsElementActive;

	// Update self
	if (bActive && Actor->IsHidden())
	{
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorEnableCollision(true);
		OnElementActivated.Broadcast(this);
	}
	else if (!bActive && !Actor->IsHidden())
	{
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		OnElementDeactivated.Broadcast(this);
	}

	// Update children
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		UUxtUIElement* UIElement = AttachedActor->FindComponentByClass<UUxtUIElement>();

		if (UIElement)
		{
			UIElement->UpdateVisibility(bActive);
		}
	}
}
