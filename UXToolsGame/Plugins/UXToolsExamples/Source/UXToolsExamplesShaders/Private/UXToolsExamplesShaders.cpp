// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UXToolsExamplesShaders.h"

#include "ShaderCore.h"

#include "Features/IModularFeatures.h"
#include "Interfaces/IPluginManager.h"

void FUXToolsExamplesShadersModule::StartupModule()
{
	// Maps virtual shader source directory /Plugin/UXToolsExamples to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("UXToolsExamples"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/UXToolsExamples"), PluginShaderDir);
}

void FUXToolsExamplesShadersModule::ShutdownModule()
{
}

IMPLEMENT_MODULE(FUXToolsExamplesShadersModule, UXToolsExamplesShaders)
