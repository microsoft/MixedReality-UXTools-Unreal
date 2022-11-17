// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "MicrosoftOpenXRRuntimeSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "CoreGlobals.h"
#include "UObject/Package.h"

UMicrosoftOpenXRRuntimeSettings* UMicrosoftOpenXRRuntimeSettings::MicrosoftOpenXRSettingsSingleton = nullptr;

#if WITH_EDITOR
void UMicrosoftOpenXRRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false);
}
#endif

UMicrosoftOpenXRRuntimeSettings* UMicrosoftOpenXRRuntimeSettings::Get()
{
	if (MicrosoftOpenXRSettingsSingleton == nullptr && GetTransientPackage() != nullptr)
	{
		static const TCHAR* SettingsContainerName = TEXT("MicrosoftOpenXRRuntimeSettingsContainer");

		MicrosoftOpenXRSettingsSingleton = FindObject<UMicrosoftOpenXRRuntimeSettings>(GetTransientPackage(), SettingsContainerName);

		if (MicrosoftOpenXRSettingsSingleton == nullptr)
		{
			MicrosoftOpenXRSettingsSingleton = NewObject<UMicrosoftOpenXRRuntimeSettings>(
				GetTransientPackage(), UMicrosoftOpenXRRuntimeSettings::StaticClass(), SettingsContainerName);
			MicrosoftOpenXRSettingsSingleton->AddToRoot();
		}
	}
	return MicrosoftOpenXRSettingsSingleton;
}

bool UMicrosoftOpenXRRuntimeSettings::ParseAddress(const FString& StringToParse, FString& Address, uint32& Port)
{
	FString PortStr;

	if (StringToParse.Len() == 0)
	{
		return false;
	}

	if (StringToParse.Split(TEXT(":"), &Address, &PortStr))
	{
		// Parse the input in format "IP:Port"
		Port = FCString::Atoi(*PortStr);
	}
	else if (!StringToParse.Contains("."))
	{
		// If the given ip is not valid, try using it as a port for a listen connection to the remoting player.
		Address = TEXT("0.0.0.0");
		Port = FCString::Atoi(*StringToParse);
	}
	else
	{
		// Otherwise only an IP is set.  Use the default port.
		Address = StringToParse;
		Port = 8265;
	}

	return true;
}

