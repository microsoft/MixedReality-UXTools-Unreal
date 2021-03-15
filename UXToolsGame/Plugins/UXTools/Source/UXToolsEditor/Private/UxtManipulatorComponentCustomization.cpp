// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtManipulatorComponentCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "UxtConstraintPickerWidget.h"

#include "Interactions/UxtManipulatorComponent.h"

void FUxtManipulatorComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Owners;
	DetailBuilder.GetObjectsBeingCustomized(Owners);
	check(Owners.Num() == 1); // There should only be one object being customized (the owning component).

	// Touch all categories to preserve initial ordering
	TArray<FName> CategoryNames;
	DetailBuilder.GetCategoryNames(CategoryNames);
	for (FName CategoryName : CategoryNames)
	{
		// Don't touch sub-categories
		if (!CategoryName.ToString().Contains("|"))
		{
			DetailBuilder.EditCategory(CategoryName);
		}
	}

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Uxt Manipulator");
	const FText Description = FText::AsCultureInvariant("Add Constraint");
	Category.AddCustomRow(Description)
		.NameContent()[SNew(STextBlock).Font(IDetailLayoutBuilder::GetDetailFont()).Text(Description)]
		.ValueContent()
		.MaxDesiredWidth(150.f)
		.MinDesiredWidth(150.f)[SNew(SButton)
									.ContentPadding(2)
									.VAlign(VAlign_Center)
									.HAlign(HAlign_Center)
									.IsEnabled(true)
									.OnClicked(
										this, &FUxtManipulatorComponentCustomization::OnShowEditor,
										Owners[0])[SNew(STextBlock)
													   .Font(IDetailLayoutBuilder::GetDetailFont())
													   .Text(FText::AsCultureInvariant("Open Constraint Picker"))]];
}

TSharedRef<IDetailCustomization> FUxtManipulatorComponentCustomization::MakeInstance()
{
	return MakeShared<FUxtManipulatorComponentCustomization>();
}

FReply FUxtManipulatorComponentCustomization::OnShowEditor(TWeakObjectPtr<UObject> Owner)
{
	if (UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(
			StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, TEXT("/UXTools/Utilities/ConstraintPicker/UxtConstraintPicker"))))
	{
		if (UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>())
		{
			FName TabID;
			// Note the use of static_cast<> from UWidgetBlueprint to UEditorUtilityWidgetBlueprint because UEditorUtilityWidgetBlueprint is
			// not exported by the BLUTILITY_API and the StaticClass method required by Cast<> cannot be used.
			if (UUxtConstraintPickerWidget* Widget = Cast<UUxtConstraintPickerWidget>(
					EditorUtilitySubsystem->SpawnAndRegisterTabAndGetID(static_cast<UEditorUtilityWidgetBlueprint*>(Blueprint), TabID)))
			{
				Widget->SetTabID(TabID);
				Widget->SetOwner(Owner);
			}
		}
	}

	return FReply::Handled();
}
