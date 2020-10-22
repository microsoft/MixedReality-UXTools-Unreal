// Copyright (c) Microsoft Corporation. All rights reserved.

#include "MRPlatExtRuntimeSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "CoreGlobals.h"
#include "UObject/Package.h"

UMRPlatExtRuntimeSettings* UMRPlatExtRuntimeSettings::MRPlatExtSettingsSingleton = nullptr;

#if WITH_EDITOR
void UMRPlatExtRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false);
}
#endif

UMRPlatExtRuntimeSettings* UMRPlatExtRuntimeSettings::Get()
{
	if (MRPlatExtSettingsSingleton == nullptr && GetTransientPackage() != nullptr)
	{
		static const TCHAR* SettingsContainerName = TEXT("MRPlatExtRuntimeSettingsContainer");

		MRPlatExtSettingsSingleton = FindObject<UMRPlatExtRuntimeSettings>(GetTransientPackage(), SettingsContainerName);

		if (MRPlatExtSettingsSingleton == nullptr)
		{
			MRPlatExtSettingsSingleton = NewObject<UMRPlatExtRuntimeSettings>(
				GetTransientPackage(), UMRPlatExtRuntimeSettings::StaticClass(), SettingsContainerName);
			MRPlatExtSettingsSingleton->AddToRoot();
		}
	}
	return MRPlatExtSettingsSingleton;
}

bool UMRPlatExtRuntimeSettings::ParseAddress(const FString& StringToParse, FString& Address, uint32& Port)
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

