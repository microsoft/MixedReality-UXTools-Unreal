// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtConstraintPickerWidget.h"

#include "SSCSEditor.h"

void UUxtConstraintPickerWidget::SetTabID(FName NewID)
{
	TabID = NewID;
}

void UUxtConstraintPickerWidget::SetOwner(TWeakObjectPtr<UObject> Owner)
{
	if (UActorComponent* Component = Cast<UActorComponent>(Owner.Get()))
	{
		// Create an editor for the parent actor / blueprint.
		if (AActor* Actor = Component->GetOwner())
		{
			SCSEditor = SNew(SSCSEditor).EditorMode(EComponentEditorMode::ActorInstance).ActorContext(Actor);
		}
		else if (UBlueprintGeneratedClass* Blueprint = Cast<UBlueprintGeneratedClass>(Component->GetOuter()))
		{
			SCSEditor = SNew(SSCSEditor).EditorMode(EComponentEditorMode::BlueprintSCS).ActorContext(Blueprint->GetDefaultObject(true));
		}
	}

	OnOwnerChanged();
}

bool UUxtConstraintPickerWidget::HasValidOwner() const
{
	if (SCSEditor.IsValid())
	{
		return IsValid(SCSEditor->GetActorContext());
	}

	return false;
}

TArray<TSubclassOf<UUxtTransformConstraint>> UUxtConstraintPickerWidget::GetConstraintClasses() const
{
	TArray<TSubclassOf<UUxtTransformConstraint>> ConstraintClasses;

	for (TObjectIterator<UClass> Class; Class; ++Class)
	{
		if (Class->IsChildOf<UUxtTransformConstraint>() && *Class != UUxtTransformConstraint::StaticClass())
		{
			ConstraintClasses.Add(*Class);
		}
	}

	return ConstraintClasses;
}

FString UUxtConstraintPickerWidget::GetConstraintName(TSubclassOf<UUxtTransformConstraint> ConstraintClass)
{
	return FName::NameToDisplayString(ConstraintClass->GetName(), false);
}

FString UUxtConstraintPickerWidget::GetConstraintDescription(TSubclassOf<UUxtTransformConstraint> ConstraintClass)
{
	check(ConstraintClass);

	// The tooltip metadata is the class comment without the comment syntax.
	return ConstraintClass->GetMetaData("Tooltip");
}

bool UUxtConstraintPickerWidget::AddConstraint(TSubclassOf<UUxtTransformConstraint> ConstraintClass)
{
	check(ConstraintClass);

	if (HasValidOwner())
	{
		if (UActorComponent* Component = SCSEditor->AddNewComponent(ConstraintClass, nullptr))
		{
			// Refresh selection if modifying an actor instance.
			if (AActor* Actor = Component->GetOwner())
			{
				GEditor->ResetAllSelectionSets();
				GEditor->SelectActor(Actor, true, true);
				GEditor->SelectComponent(Component, true, true);
			}

			return true;
		}
	}

	return false;
}
