// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UXTools.h"
#include "UxtDefaultHandTracker.h"

#define LOCTEXT_NAMESPACE "UXToolsInputModule"

/**
 * Module for XR input handling.
 *
 * Note: Input mappings should be generated in the PostEngineInit loading phase,
 * which is why a separate input module is required. This is because we need
 * to be sure that all input keys have been registered before trying to create mappings.
 */
class FUXToolsInputModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUXToolsInputModule, UXToolsInput)
