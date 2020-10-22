// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"

#include "MRPlatExtRuntimeSettings.generated.h"

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

namespace MRPlatExt
{
	struct RemotingConnectionData
	{
		FString IP;
		uint32 Port = 8265;
		int Bitrate = 8000;
		bool EnableAudio = false;
		RemotingConnectionType ConnectionType = RemotingConnectionType::Connect;
		RemotingCodec ConnectionCodec = RemotingCodec::Any;
	};
}	 // namespace MRPlatExt

DECLARE_DELEGATE_TwoParams(FMRPlatExtRemotingStatusChanged, FString /*RemotingMessage*/, FLinearColor /*StatusColor*/);
DECLARE_DELEGATE_OneParam(FMRPlatExtRemotingConnect, MRPlatExt::RemotingConnectionData);
DECLARE_DELEGATE(FMRPlatExtRemotingDisconnect);

/**
 * Implements the settings for the WindowsMixedReality runtime platform.
 */
UCLASS(config=EditorPerProjectUserSettings)
class MRPLATEXTRUNTIMESETTINGS_API UMRPlatExtRuntimeSettings : public UObject
{
public:
	GENERATED_BODY()

	FMRPlatExtRemotingStatusChanged OnRemotingStatusChanged;
	FMRPlatExtRemotingConnect OnRemotingConnect;
	FMRPlatExtRemotingDisconnect OnRemotingDisconnect;

	static UMRPlatExtRuntimeSettings* Get();

	static bool ParseAddress(const FString& StringToParse, FString& Address, uint32& Port);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif	  // WITH_EDITOR

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (ConfigRestartRequired = true, DisplayName = "Enable Remoting For Editor (Requires Restart)", Tooltip = "If true, start with a valid HMD to enable connecting via remoting.  Editor restart required."))
	bool bEnableRemotingForEditor = false;

	/** The IP of the HoloLens to remote to. */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "IP of HoloLens to remote to."))
	FString RemoteHoloLensIP;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Max network transfer rate (kb/s)."))
	unsigned int MaxBitrate = 8000;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Use audio from PC when remoting."))
	bool EnableAudio = false;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Connection Type."))
	RemotingConnectionType ConnectionType = RemotingConnectionType::Connect;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = "OpenXR Holographic Remoting", Meta = (EditCondition = "bEnableRemotingForEditor", DisplayName = "Connection Codec."))
	RemotingCodec ConnectionCodec = RemotingCodec::Any;

private:
	static class UMRPlatExtRuntimeSettings* MRPlatExtSettingsSingleton;
};
