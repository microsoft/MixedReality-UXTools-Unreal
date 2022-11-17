// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"

#include "MicrosoftOpenXRRuntimeSettings.generated.h"

UENUM()
enum class RemotingConnectionType
{
	Connect = 0,
	Listen = 1
};

// This will be cast to XrRemotingVideoCodecMSFT, IDs must match
UENUM()
enum class RemotingCodec
{
	Any = 0,
	H264 = 1,
	H265 = 2
};

namespace MicrosoftOpenXR
{
	struct RemotingConnectionData
	{
		FString IP;
		uint32 Port = 8265;
		int Bitrate = 20000;
		bool EnableAudio = false;
		RemotingConnectionType ConnectionType = RemotingConnectionType::Connect;
		RemotingCodec ConnectionCodec = RemotingCodec::Any;
	};
}	 // namespace MicrosoftOpenXR

DECLARE_DELEGATE_TwoParams(FMicrosoftOpenXRRemotingStatusChanged, FString /*RemotingMessage*/, FLinearColor /*StatusColor*/);
DECLARE_DELEGATE_OneParam(FMicrosoftOpenXRRemotingConnect, MicrosoftOpenXR::RemotingConnectionData);
DECLARE_DELEGATE(FMicrosoftOpenXRRemotingDisconnect);

/**
 * Implements the settings for Microsoft OpenXR runtime platforms.
 */
UCLASS(config=EditorPerProjectUserSettings)
class MICROSOFTOPENXRRUNTIMESETTINGS_API UMicrosoftOpenXRRuntimeSettings : public UObject
{
public:
	GENERATED_BODY()

	FMicrosoftOpenXRRemotingStatusChanged OnRemotingStatusChanged;
	FMicrosoftOpenXRRemotingConnect OnRemotingConnect;
	FMicrosoftOpenXRRemotingDisconnect OnRemotingDisconnect;

	static UMicrosoftOpenXRRuntimeSettings* Get();

	static bool ParseAddress(const FString& StringToParse, FString& Address, uint32& Port);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif	  // WITH_EDITOR

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (ConfigRestartRequired = true, DisplayName = "Enable Remoting For Editor (Requires Restart)", Tooltip = "If true, start with a valid HMD to enable connecting via remoting.  Editor restart required."))
	bool bEnableRemotingForEditor = false;

	/** The IP of the HoloLens or WMR HMD to remote to. */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Remote Device IP."))
	FString RemoteHoloLensIP;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Automatically connect to remote device."))
	bool bAutoConnectRemoting = false;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Max network transfer rate (kb/s)."))
	unsigned int MaxBitrate = 20000;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Use audio from PC when remoting."))
	bool EnableAudio = false;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Connection Type."))
	RemotingConnectionType ConnectionType = RemotingConnectionType::Connect;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Connection Codec."))
	RemotingCodec ConnectionCodec = RemotingCodec::Any;

private:
	static class UMicrosoftOpenXRRuntimeSettings* MicrosoftOpenXRSettingsSingleton;
};
