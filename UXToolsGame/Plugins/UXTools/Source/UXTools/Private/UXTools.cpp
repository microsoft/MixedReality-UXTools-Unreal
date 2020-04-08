// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UXTools.h"

DEFINE_LOG_CATEGORY(UXTools)

#define LOCTEXT_NAMESPACE "UXToolsModule"

void FUXToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUXToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUXToolsModule, UXTools)