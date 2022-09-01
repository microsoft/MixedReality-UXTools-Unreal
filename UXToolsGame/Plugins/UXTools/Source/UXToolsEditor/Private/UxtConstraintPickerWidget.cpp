// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtConstraintPickerWidget.h"

#include "SSubobjectBlueprintEditor.h"
#include "SSubobjectInstanceEditor.h"
#include "UXToolsEditor.h"

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
			SubobjectEditor = SNew(SSubobjectInstanceEditor).ObjectContext(Actor);
			IsBlueprint = false;
		}
		else if (UBlueprintGeneratedClass* Blueprint = Cast<UBlueprintGeneratedClass>(Component->GetOuter()))
		{
			SubobjectEditor = SNew(SSubobjectBlueprintEditor).ObjectContext(Blueprint->GetDefaultObject(true));
			IsBlueprint = true;
		}
	}

	OnOwnerChanged();
}

bool UUxtConstraintPickerWidget::HasValidOwner() const
{
	if (SubobjectEditor.IsValid())
	{
		return IsValid(SubobjectEditor->GetObjectContext());
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
		FAddNewSubobjectParams ComponentParams;
		ComponentParams.ParentHandle = SubobjectEditor->GetObjectContextHandle();
		ComponentParams.NewClass = ConstraintClass;
		ComponentParams.BlueprintContext = IsBlueprint ? SubobjectEditor->GetBlueprint() : nullptr;

		FText FailReason;
		FSubobjectDataHandle ComponentHandle = USubobjectDataSubsystem::Get()->AddNewSubobject(ComponentParams, FailReason);
		if (!ComponentHandle.IsValid())
		{
			UE_LOG(UXToolsEditor, Error, TEXT("Failed to add component: %s"), *FailReason.ToString());
			return false;
		}

		if (!IsBlueprint)
		{
			// We can only get a const pointer to the new component through the ComponentHandle
			// so we have to get it through the actor instead.
			if (AActor* Actor = Cast<AActor>(SubobjectEditor->GetObjectContext()))
			{
				TArray<UActorComponent*> Components;
				Actor->GetComponents(ConstraintClass, Components);
				if (!Components.IsEmpty())
				{
					UActorComponent* Component = Components.Last(); // The new component will always be last.
					GEditor->ResetAllSelectionSets();
					GEditor->SelectActor(Actor, true, true);
					GEditor->SelectComponent(Component, true, true);
				}
			}
		}

		return true;
	}

	return false;
}
