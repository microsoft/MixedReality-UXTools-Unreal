// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtRuntimeSettings.h"

#include "CoreGlobals.h"

#include "Misc/ConfigCacheIni.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"

UUxtRuntimeSettings* UUxtRuntimeSettings::UXToolsSettingsSingleton = nullptr;

UUxtRuntimeSettings::UUxtRuntimeSettings(const FObjectInitializer& ObjectInitializer)
{
}

#if WITH_EDITOR

void UUxtRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false);
}

#endif

UUxtRuntimeSettings* UUxtRuntimeSettings::Get()
{
	if (UXToolsSettingsSingleton == nullptr)
	{
		static const TCHAR* SettingsContainerName = TEXT("UXToolsRuntimeSettingsContainer");

		UXToolsSettingsSingleton = FindObject<UUxtRuntimeSettings>(GetTransientPackage(), SettingsContainerName);

		if (UXToolsSettingsSingleton == nullptr)
		{
			UXToolsSettingsSingleton =
				NewObject<UUxtRuntimeSettings>(GetTransientPackage(), UUxtRuntimeSettings::StaticClass(), SettingsContainerName);
			UXToolsSettingsSingleton->AddToRoot();
		}
	}
	return UXToolsSettingsSingleton;
}