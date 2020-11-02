// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRInputSimulationEditor.h"

#include "ISettingsModule.h"
#include "XRInputSimulationRuntimeSettings.h"

IMPLEMENT_GAME_MODULE(FXRInputSimulationEditorModule, XRInputSimulationEditor);

DEFINE_LOG_CATEGORY(XRInputSimulationEditor)

#define LOCTEXT_NAMESPACE "XRInputSimulationEditor"

void FXRInputSimulationEditorModule::StartupModule()
{
	// Register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		{
			SettingsModule->RegisterSettings(
				"Project", "Plugins", "XRInputSimulation", LOCTEXT("RuntimeSettingsName", "XR Input Simulation"),
				LOCTEXT("RuntimeSettingsDescription", "Project settings for XR Input Simulation"),
				GetMutableDefault<UXRInputSimulationRuntimeSettings>());
		}
	}
}

void FXRInputSimulationEditorModule::ShutdownModule()
{
	// Unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "XRInputSimulation");
	}
}

#undef LOCTEXT_NAMESPACE
