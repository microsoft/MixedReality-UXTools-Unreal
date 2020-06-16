// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UXToolsEditor.h"
#include "UxtPressableButtonComponentVisualizer.h"
#include "UxtIconBrushCustomization.h"
#include "Controls/UxtIconBrush.h"
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
		// Register visualizers
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FUxtPressableButtonComponentVisualizer());

		if (Visualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(UUxtPressableButtonComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
	}

	// Register customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(FUxtIconBrush::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUxtIconBrushCustomization::MakeInstance));

	// Register settings
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
		// Unregister visualizers
		GUnrealEd->UnregisterComponentVisualizer(UUxtPressableButtonComponent::StaticClass()->GetFName());
	}

	// Unregister customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomPropertyTypeLayout(FUxtIconBrush::StaticStruct()->GetFName());

	// Unregister settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "UXTools");
	}
}

#undef LOCTEXT_NAMESPACE
