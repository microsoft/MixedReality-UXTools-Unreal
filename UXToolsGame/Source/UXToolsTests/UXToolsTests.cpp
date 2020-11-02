// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UXToolsTests.h"

DEFINE_LOG_CATEGORY(UXToolsTests)

#define LOCTEXT_NAMESPACE "UXToolsTestsModule"

void FUXToolsTestsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FUXToolsTestsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUXToolsTestsModule, UXToolsTests)
