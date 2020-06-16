// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtIconBrushEditorUtilityWidget.h"
#include "Controls/UxtIconBrush.h"
#include "Engine/Font.h"

static const FName IconBrushStringName = GET_MEMBER_NAME_CHECKED(FUxtIconBrush, IconString);
static const FName IconBrushFontName = GET_MEMBER_NAME_CHECKED(FUxtIconBrush, IconFont);

UFont* UUxtIconBrushEditorUtilityWidget::GetIconBrushFont() const
{
	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		UObject* Object = nullptr;
		if (LockedObserver->GetChildHandle(IconBrushFontName)->GetValue(Object) == FPropertyAccess::Success)
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
		return (LockedObserver->GetChildHandle(IconBrushFontName)->SetValue(Font) == FPropertyAccess::Success);
	}

	return false;
}

bool UUxtIconBrushEditorUtilityWidget::GetIconBrushString(FString& IconString) const
{
	IconString.Empty();

	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		return (LockedObserver->GetChildHandle(IconBrushStringName)->GetValue(IconString) == FPropertyAccess::Success);
	}

	return false;
}

bool UUxtIconBrushEditorUtilityWidget::SetIconBrushString(const FString& IconString)
{
	if (TSharedPtr<IPropertyHandle> LockedObserver = PropertyHandle.Pin())
	{
		return (LockedObserver->GetChildHandle(IconBrushStringName)->SetValue(IconString) == FPropertyAccess::Success);
	}

	return false;
}
