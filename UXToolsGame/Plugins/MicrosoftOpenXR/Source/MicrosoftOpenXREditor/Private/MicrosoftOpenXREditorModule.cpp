// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "MicrosoftOpenXRRuntimeSettings.h"
#include "Modules/ModuleInterface.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "MicrosoftOpenXRDetails.h"

#define LOCTEXT_NAMESPACE "FMicrosoftOpenXREditorModule"


/**
 * Module for MicrosoftOpenXR platform editor utilities
 */
class FMicrosoftOpenXREditorModule
	: public IModuleInterface
{
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(FName("MicrosoftOpenXRRuntimeSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FMicrosoftOpenXRDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		// register settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "MicrosoftOpenXR",
				// Using "Windows Mixed Reality" here to preserve the remoting location from legacy WMR in Project Settings/ Windows Mixed Reality
				LOCTEXT("RuntimeSettingsName", "Windows Mixed Reality"),
				LOCTEXT("RuntimeSettingsDescription", "Project settings for Mixed Reality Platform Extensions"),
				GetMutableDefault<UMicrosoftOpenXRRuntimeSettings>()
			);
		}
	}

	virtual void ShutdownModule() override
	{
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "MicrosoftOpenXR");
		}
	}
};


IMPLEMENT_MODULE(FMicrosoftOpenXREditorModule, MicrosoftOpenXREditor);

#undef LOCTEXT_NAMESPACE
