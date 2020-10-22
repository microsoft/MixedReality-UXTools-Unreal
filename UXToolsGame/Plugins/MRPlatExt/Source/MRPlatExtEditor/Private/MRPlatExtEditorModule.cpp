// Copyright (c) Microsoft Corporation. All rights reserved.

#include "MRPlatExtRuntimeSettings.h"
#include "Modules/ModuleInterface.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "MRPlatExtDetails.h"

#define LOCTEXT_NAMESPACE "FMRPlatExtEditorModule"


/**
 * Module for MRPlatExt platform editor utilities
 */
class FMRPlatExtEditorModule
	: public IModuleInterface
{
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(FName("MRPlatExtRuntimeSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FMRPlatExtDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		// register settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "MRPlatExt",
				// Using "Windows Mixed Reality" here to preserve the remoting location from legacy WMR in Project Settings/ Windows Mixed Reality
				LOCTEXT("RuntimeSettingsName", "Windows Mixed Reality"),
				LOCTEXT("RuntimeSettingsDescription", "Project settings for Mixed Reality Platform Extensions"),
				GetMutableDefault<UMRPlatExtRuntimeSettings>()
			);
		}
	}

	virtual void ShutdownModule() override
	{
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "MRPlatExt");
		}
	}
};


IMPLEMENT_MODULE(FMRPlatExtEditorModule, MRPlatExtEditor);

#undef LOCTEXT_NAMESPACE
