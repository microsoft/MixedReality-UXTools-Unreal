// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UXToolsEditor.h"

#include "ISettingsModule.h"
#include "UnrealEdGlobals.h"
#include "UxtIconBrushCustomization.h"
#include "UxtPressableButtonComponentVisualizer.h"
#include "UxtRuntimeSettings.h"
#include "UxtTooltipSpawnerComponentVisualizer.h"

#include "Controls/UxtIconBrush.h"
#include "Editor/UnrealEdEngine.h"
#include "Tooltips/UxtTooltipSpawnerComponent.h"

IMPLEMENT_GAME_MODULE(FUXToolsEditorModule, UXToolsEditor);

DEFINE_LOG_CATEGORY(UXToolsEditor)

#define LOCTEXT_NAMESPACE "UXToolsEditor"

void FUXToolsEditorModule::StartupModule()
{
	if (GUnrealEd)
	{
		// Register visualizers
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FUxtPressableButtonComponentVisualizer());
		TSharedPtr<FComponentVisualizer> TooltipVisualizer = MakeShareable(new FUxtTooltipSpawnerComponentVisualizer());

		if (Visualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(UUxtPressableButtonComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
		if (TooltipVisualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(UUxtTooltipSpawnerComponent::StaticClass()->GetFName(), TooltipVisualizer);
			TooltipVisualizer->OnRegister();
		}
	}

	// Register customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FUxtIconBrush::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FUxtIconBrushCustomization::MakeInstance));

	// Register settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		{
			SettingsModule->RegisterSettings(
				"Project", "Plugins", "UXTools", LOCTEXT("RuntimeSettingsName", "UX Tools"),
				LOCTEXT("RuntimeSettingsDescription", "Project settings for UX Tools"), GetMutableDefault<UUxtRuntimeSettings>());
		}
	}
}

void FUXToolsEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		// Unregister visualizers
		GUnrealEd->UnregisterComponentVisualizer(UUxtTooltipSpawnerComponent::StaticClass()->GetFName());
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
