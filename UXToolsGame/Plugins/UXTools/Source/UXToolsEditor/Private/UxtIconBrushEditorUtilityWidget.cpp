// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtIconBrushEditorUtilityWidget.h"

#include "Controls/UxtIconBrush.h"
#include "Engine/Font.h"

static const FName IconBrushIconName = GET_MEMBER_NAME_CHECKED(FUxtIconBrush, Icon);
static const FName IconBrushTextBrushName = GET_MEMBER_NAME_CHECKED(FUxtIconBrush, TextBrush);
static const FName TextBrushFontName = GET_MEMBER_NAME_CHECKED(FUxtTextBrush, Font);

UFont* UUxtIconBrushEditorUtilityWidget::GetIconBrushFont() const
{
	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		UObject* Object = nullptr;
		if (LockedObserver->GetChildHandle(IconBrushTextBrushName)->GetChildHandle(TextBrushFontName)->GetValue(Object) ==
			FPropertyAccess::Success)
		{
			return Cast<UFont>(Object);
		}
	}

	return nullptr;
}

bool UUxtIconBrushEditorUtilityWidget::SetIconBrushFont(const UFont* Font)
{
	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		return (
			LockedObserver->GetChildHandle(IconBrushTextBrushName)->GetChildHandle(TextBrushFontName)->SetValue(Font) ==
			FPropertyAccess::Success);
	}

	return false;
}

bool UUxtIconBrushEditorUtilityWidget::GetIconBrushString(FString& IconString) const
{
	IconString.Empty();

	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		return (LockedObserver->GetChildHandle(IconBrushIconName)->GetValue(IconString) == FPropertyAccess::Success);
	}

	return false;
}

bool UUxtIconBrushEditorUtilityWidget::SetIconBrushString(const FString& IconString)
{
	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		return (LockedObserver->GetChildHandle(IconBrushIconName)->SetValue(IconString) == FPropertyAccess::Success);
	}

	return false;
}
