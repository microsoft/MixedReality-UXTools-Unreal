#include "MixedRealityUtilsEditor.h"
#include "PressableButtonComponentVisualizer.h"

IMPLEMENT_GAME_MODULE(FMixedRealityUtilsEditorModule, MixedRealityUtilsEditor);

DEFINE_LOG_CATEGORY(MixedRealityUtilsEditor)

#define LOCTEXT_NAMESPACE "MixedRealityUtilsEditor"


void FMixedRealityUtilsEditorModule::StartupModule()
{
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
}

#undef LOCTEXT_NAMESPACE