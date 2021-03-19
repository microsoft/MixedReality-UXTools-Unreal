// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

#include "Input/Reply.h"

/**
 * Provides a custom property panel for the UxtManipulatorComponent.
 */
class FUxtManipulatorComponentCustomization : public IDetailCustomization
{
public:
	// IDetailCustomization interface.
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	FReply OnShowEditor(TWeakObjectPtr<UObject> Owner);
};
