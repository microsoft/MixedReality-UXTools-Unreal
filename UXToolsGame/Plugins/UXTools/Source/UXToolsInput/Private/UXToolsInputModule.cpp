// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UXTools.h"

#include "UxtDefaultHandTracker.h"

#define LOCTEXT_NAMESPACE "UXToolsInputModule"

/**
 * Module for XR input handling.
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
