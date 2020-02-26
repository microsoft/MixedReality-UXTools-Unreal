// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "UXToolsWMR.h"
#include "Features/IModularFeatures.h"
#include "HandTracking/UxtWmrHandTracker.h"

static FUxtWmrHandTracker WmrHandTracker;

void FUXToolsWMRModule::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &WmrHandTracker);
}

void FUXToolsWMRModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &WmrHandTracker);
}
	
IMPLEMENT_MODULE(FUXToolsWMRModule, UXToolsWMR)