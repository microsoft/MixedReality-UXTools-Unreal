// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "ISettingsModule.h"
#include "XRSimulationRuntimeSettings.h"

DEFINE_LOG_CATEGORY_STATIC(XRSimulationEditor, All, All)

#define LOCTEXT_NAMESPACE "XRSimulationEditor"

class FXRSimulationEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FXRSimulationEditorModule, XRSimulationEditor);

void FXRSimulationEditorModule::StartupModule()
{
	// Register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		{
			SettingsModule->RegisterSettings(
				"Project", "Plugins", "XRSimulation", LOCTEXT("RuntimeSettingsName", "XR Simulation"),
				LOCTEXT("RuntimeSettingsDescription", "Project settings for XR Simulation"),
				GetMutableDefault<UXRSimulationRuntimeSettings>());
		}
	}
}

void FXRSimulationEditorModule::ShutdownModule()
{
	// Unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "XRSimulation");
	}
}

#undef LOCTEXT_NAMESPACE
