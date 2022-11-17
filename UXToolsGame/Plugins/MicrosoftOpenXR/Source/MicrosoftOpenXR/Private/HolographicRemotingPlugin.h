// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "MicrosoftOpenXR.h"

#if SUPPORTS_REMOTING

#include "Interfaces/IPluginManager.h"

#include "OpenXRCommon.h"
#include "openxr_msft_holographic_remoting.h "

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "OpenXRCore.h"
#include "HeadMountedDisplayTypes.h"

#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "MicrosoftOpenXRRuntimeSettings.h"

#include "Misc/EngineVersionComparison.h"

// Microsoft.Holographic.AppRemoting binaries only exist for Win64.
#define SUPPORTS_REMOTING_IN_PACKAGED_BUILD (!WITH_EDITOR && PLATFORM_DESKTOP && PLATFORM_64BITS)

namespace MicrosoftOpenXR
{
	class FHolographicRemotingPlugin : public IOpenXRExtensionPlugin, public TSharedFromThis<FHolographicRemotingPlugin>
	{
	public:
		void Register();
		void Unregister();

		bool GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr) override;
		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
		void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;
		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;

		bool IsRemoting()
		{
			return remotingEnabled;
		}

	private:
		void* LoaderHandle;
		bool remotingEnabled = false;
		bool autoConnectRemoting = false;

		XrInstance Instance;
		XrSystemId System = XR_NULL_SYSTEM_ID;

		RemotingConnectionData remotingConnectionData;

		XrRemotingConnectionStateMSFT GetRemotingConnectionState();
		void BindConnectButtons();
		void ConnectToRemoteDevice(RemotingConnectionData data);
		void DisconnectFromRemoteDevice();

		void SetRemotingStatusText(FString message, FLinearColor statusColor);
		void UpdateDisconnectedText();
		FString GetDisconnectedReason(int index);

		void ParseRemotingConfig();
		bool ParseRemotingCmdArgs();
	};
}	 // namespace MicrosoftOpenXR

#endif
