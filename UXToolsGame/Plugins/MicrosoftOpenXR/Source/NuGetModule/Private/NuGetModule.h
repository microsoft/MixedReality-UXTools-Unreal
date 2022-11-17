// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FNuGetModule : public IModuleInterface
{
public:
	void StartupModule() override { }
	void ShutdownModule() override { }
};
