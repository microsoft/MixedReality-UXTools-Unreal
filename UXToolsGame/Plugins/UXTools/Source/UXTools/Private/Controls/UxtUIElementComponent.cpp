// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtUIElementComponent.h"

#include "GameFramework/Actor.h"

EUxtUIElementVisibility UUxtUIElementComponent::GetUIVisibilitySelf() const
{
	return Visibility;
}

EUxtUIElementVisibility UUxtUIElementComponent::GetUIVisibilityInHierarchy() const
{
	const EUxtUIElementVisibility ParentVisibility = GetParentVisibility();

	if (ParentVisibility != EUxtUIElementVisibility::Show)
	{
		return ParentVisibility;
	}

	return Visibility;
}

void UUxtUIElementComponent::SetUIVisibility(EUxtUIElementVisibility NewVisibility)
{
	if (NewVisibility != Visibility)
	{
		Visibility = NewVisibility;

		RefreshUIElement();
	}
}

void UUxtUIElementComponent::RefreshUIElement()
{
	UpdateVisibility(GetParentVisibility());
}

void UUxtUIElementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Visibility != EUxtUIElementVisibility::Show)
	{
		UpdateVisibility();
	}
}

void UUxtUIElementComponent::OnAttachmentChanged()
{
	RefreshUIElement();
}

EUxtUIElementVisibility UUxtUIElementComponent::GetParentVisibility() const
{
	const AActor* Parent = GetOwner()->GetAttachParentActor();

	if (Parent)
	{
		const UUxtUIElementComponent* ParentUIElement = Parent->FindComponentByClass<UUxtUIElementComponent>();

		if (ParentUIElement)
		{
			return ParentUIElement->GetUIVisibilityInHierarchy();
		}
	}

	return EUxtUIElementVisibility::Show;
}

void UUxtUIElementComponent::UpdateVisibility(EUxtUIElementVisibility ParentVisibility)
{
	AActor* Actor = GetOwner();
	const EUxtUIElementVisibility CurrentVisiblity = ParentVisibility == EUxtUIElementVisibility::Show ? Visibility : ParentVisibility;

	// Update self
	if (CurrentVisiblity == EUxtUIElementVisibility::Show && Actor->IsHidden())
	{
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorEnableCollision(true);

		OnShowElement.Broadcast(this);
	}
	else if (CurrentVisiblity != EUxtUIElementVisibility::Show && !Actor->IsHidden())
	{
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);

		const bool bShouldAffectLayout = CurrentVisiblity == EUxtUIElementVisibility::LayoutOnly;
		OnHideElement.Broadcast(this, bShouldAffectLayout);
	}

	// Update children
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors);

	for (AActor* AttachedActor : AttachedActors)
	{
		UUxtUIElementComponent* UIElement = AttachedActor->FindComponentByClass<UUxtUIElementComponent>();

		if (UIElement)
		{
			UIElement->UpdateVisibility(CurrentVisiblity);
		}
	}
}
