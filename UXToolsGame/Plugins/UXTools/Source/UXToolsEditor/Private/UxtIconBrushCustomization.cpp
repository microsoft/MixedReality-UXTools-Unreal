// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtIconBrushCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "IDetailChildrenBuilder.h"
#include "UxtIconBrushEditorUtilityWidget.h"

#include "Controls/UxtIconBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FUxtIconBrushCustomization::MakeInstance()
{
	return MakeShareable(new FUxtIconBrushCustomization());
}

void FUxtIconBrushCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Display the default header.
	HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()];
}

void FUxtIconBrushCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildProps = 0;
	PropertyHandle->GetNumChildren(NumChildProps);

	for (uint32 Idx = 0; Idx < NumChildProps; ++Idx)
	{
		TSharedPtr<IPropertyHandle> PropHandle = PropertyHandle->GetChildHandle(Idx);

		// Under the IconString, display a button to open the character map viewer.
		FName PropertyName = PropHandle->GetProperty()->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FUxtIconBrush, Icon))
		{
			Builder.AddProperty(PropHandle.ToSharedRef());

			const FText Description = FText::AsCultureInvariant("Edit");
			Builder.AddCustomRow(Description)
				.NameContent()[SNew(STextBlock).Font(IDetailLayoutBuilder::GetDetailFont()).Text(Description)]
				.ValueContent()
				.MaxDesiredWidth(150.f)
				.MinDesiredWidth(150.f)[SNew(SButton)
											.ContentPadding(2)
											.VAlign(VAlign_Center)
											.HAlign(HAlign_Center)
											.IsEnabled((MakeAttributeLambda([=] {
												// Enable the button only when the IconBrushContentType is set to UnicodeCharacter or String.
												uint8 Enum = 0;
												const FName ChildName = GET_MEMBER_NAME_CHECKED(FUxtIconBrush, ContentType);
												const bool Success =
													PropertyHandle->GetChildHandle(ChildName)->GetValue(Enum) == FPropertyAccess::Success;
												return Success &&
													   (static_cast<EUxtIconBrushContentType>(Enum) ==
															EUxtIconBrushContentType::UnicodeCharacter ||
														static_cast<EUxtIconBrushContentType>(Enum) == EUxtIconBrushContentType::String);
											})))
											.OnClicked(
												this, &FUxtIconBrushCustomization::OnShowEditor,
												PropertyHandle)[SNew(STextBlock)
																	.Font(IDetailLayoutBuilder::GetDetailFont())
																	.Text(FText::AsCultureInvariant("Open Icon Brush Editor"))]];
		}
		else
		{
			// Else, simply display the property.
			Builder.AddProperty(PropHandle.ToSharedRef());
		}
	}
}

FReply FUxtIconBrushCustomization::OnShowEditor(TSharedRef<IPropertyHandle> PropertyHandle)
{
	if (UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(
			StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, TEXT("/UXTools/Utilities/IconBrushEditor/UW_IconBrushEditor"))))
	{
		if (UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>())
		{
			FName TabID;
			// Note the use of static_cast<> from UWidgetBlueprint to UEditorUtilityWidgetBlueprint because UEditorUtilityWidgetBlueprint is
			// not exported by the BLUTILITY_API and the StaticClass method required by Cast<> cannot be used.
			if (UUxtIconBrushEditorUtilityWidget* Widget = Cast<UUxtIconBrushEditorUtilityWidget>(
					EditorUtilitySubsystem->SpawnAndRegisterTabAndGetID(static_cast<UEditorUtilityWidgetBlueprint*>(Blueprint), TabID)))
			{
				Widget->SetTabID(TabID);
				Widget->SetPropertyHandle(PropertyHandle);
			}
		}
	}

	return FReply::Handled();
}
