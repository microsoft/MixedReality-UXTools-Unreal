// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

#include "Input/Reply.h"

class IPropertyHandle;

/**
 * Provides a custom property panel for the UxtIconBrush.
 */
class FUxtIconBrushCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

protected:
	// IDetailCustomization interface
	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
	FReply OnShowEditor(TSharedRef<IPropertyHandle> PropertyHandle);
};
