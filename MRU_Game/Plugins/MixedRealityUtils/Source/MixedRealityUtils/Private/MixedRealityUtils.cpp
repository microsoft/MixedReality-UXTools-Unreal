// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityUtils.h"

DEFINE_LOG_CATEGORY(MixedRealityUtils)

#define LOCTEXT_NAMESPACE "FMixedRealityUtilsModule"

void FMixedRealityUtilsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FMixedRealityUtilsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

UMaterialInterface* FMixedRealityUtilsModule::GetDefaultCursorRingMaterial()
{
	return Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, TEXT("/MixedRealityUtils/Pointers/FingerCursorMaterial")));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMixedRealityUtilsModule, MixedRealityUtils)