// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UXToolsEditor.h"
#include "UxtPressableButtonComponentVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "ISettingsModule.h"
#include "UxtRuntimeSettings.h"

IMPLEMENT_GAME_MODULE(FUXToolsEditorModule, UXToolsEditor);

DEFINE_LOG_CATEGORY(UXToolsEditor)

#define LOCTEXT_NAMESPACE "UXToolsEditor"


void FUXToolsEditorModule::StartupModule()
{
	if (GUnrealEd)
	{
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FUxtPressableButtonComponentVisualizer());

		if (Visualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(UUxtPressableButtonComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
	}

	// register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		{
			SettingsModule->RegisterSettings("Project", "Plugins", "UXTools",
				LOCTEXT("RuntimeSettingsName", "UX Tools"),
				LOCTEXT("RuntimeSettingsDescription", "Project settings for UX Tools"),
				GetMutableDefault<UUxtRuntimeSettings>()
			);
		}
	}
}

void FUXToolsEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UUxtPressableButtonComponent::StaticClass()->GetFName());
	}

	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "UXTools");
	}
}

#undef LOCTEXT_NAMESPACE
