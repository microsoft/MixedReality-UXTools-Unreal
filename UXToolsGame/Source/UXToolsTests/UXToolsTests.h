// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(UXToolsTests, All, All)

class FUXToolsTestsModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
