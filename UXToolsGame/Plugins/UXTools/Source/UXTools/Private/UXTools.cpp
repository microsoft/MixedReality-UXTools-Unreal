// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UXTools.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

DEFINE_LOG_CATEGORY(UXTools)

#define LOCTEXT_NAMESPACE "UXToolsModule"

void FUXToolsModule::StartupModule()
{
	// Maps virtual shader source directory /Plugin/UXTools to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("UXTools"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/UXTools"), PluginShaderDir);
}

void FUXToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUXToolsModule, UXTools)