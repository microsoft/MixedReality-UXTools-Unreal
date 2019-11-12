#include "MixedRealityUtilsEditor.h"
#include "PressableButtonComponentVisualizer.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

IMPLEMENT_GAME_MODULE(FMixedRealityUtilsEditorModule, MixedRealityUtilsEditor);

DEFINE_LOG_CATEGORY(MixedRealityUtilsEditor)

#define LOCTEXT_NAMESPACE "MixedRealityUtilsEditor"


void FMixedRealityUtilsEditorModule::StartupModule()
{
	FModuleStatus Status;
	if (FModuleManager::Get().QueryModule("MixedRealityUtilsEditor", Status))
	{
		UE_LOG(MixedRealityUtilsEditor, Warning, TEXT("Startup from DLL %s"),*Status.FilePath);
	}
	else
	{
		UE_LOG(MixedRealityUtilsEditor, Warning, TEXT("Module not found"));
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

void FMixedRealityUtilsEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UPressableButtonComponent::StaticClass()->GetFName());
	}

	UE_LOG(MixedRealityUtilsEditor, Warning, TEXT("Shutdown"));
}

#undef LOCTEXT_NAMESPACE