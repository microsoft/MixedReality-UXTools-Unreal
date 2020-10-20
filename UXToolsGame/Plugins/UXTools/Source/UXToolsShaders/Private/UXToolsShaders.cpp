// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UXToolsShaders.h"

#include "ShaderCore.h"

#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"

void FUXToolsShadersModule::StartupModule()
{
	// Maps virtual shader source directory /Plugin/UXTools to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("UXTools"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/UXTools"), PluginShaderDir);
}

void FUXToolsShadersModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FUXToolsShadersModule, UXToolsShaders)
