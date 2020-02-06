#include "UXToolsEditor.h"
#include "UxtPressableButtonComponentVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"

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
}

void FUXToolsEditorModule::ShutdownModule()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UUxtPressableButtonComponent::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE