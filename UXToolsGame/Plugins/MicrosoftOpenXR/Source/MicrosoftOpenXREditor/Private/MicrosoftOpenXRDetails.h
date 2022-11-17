// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "IDetailCustomNodeBuilder.h"
#include "PropertyHandle.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyCustomizationHelpers.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"

#include "MicrosoftOpenXRRuntimeSettings.h"

class FMicrosoftOpenXRDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FString StatusText;
	TSharedPtr<STextBlock> statusTextWidget;

	void SetStatusText(FString message, FLinearColor statusColor);
	FReply OnConnectButtonClicked();
	FReply OnDisconnectButtonClicked();
	bool AreButtonsEnabled() const;
};

