#include "MixedRealityToolsEditor.h"
#include "PressableButtonComponentVisualizer.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

IMPLEMENT_GAME_MODULE(FMixedRealityToolsEditorModule, MixedRealityToolsEditor);

DEFINE_LOG_CATEGORY(MixedRealityToolsEditor)

#define LOCTEXT_NAMESPACE "MixedRealityToolsEditor"


void FMixedRealityToolsEditorModule::StartupModule()
{
	FModuleStatus Status;
	if (FModuleManager::Get().QueryModule("MixedRealityToolsEditor", Status))
	{
		UE_LOG(MixedRealityToolsEditor, Warning, TEXT("Startup from DLL %s"),*Status.FilePath);
	}
	else
	{
		UE_LOG(MixedRealityToolsEditor, Warning, TEXT("Module not found"));
	}

	if (GUnrealEd)
	{
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FPressableButtonComponentVisualizer());

		if (Visualizer.IsValid())
		{
			GUnrealEd->RegisterComponentVisualizer(UPressableButtonComponent::StaticClass()->GetFName(), Visualizer);
			Visualizer->OnRegister();
		}
	}
}

void FMixedRealityToolsEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UPressableButtonComponent::StaticClass()->GetFName());
	}

	UE_LOG(MixedRealityToolsEditor, Warning, TEXT("Shutdown"));
}

#undef LOCTEXT_NAMESPACE