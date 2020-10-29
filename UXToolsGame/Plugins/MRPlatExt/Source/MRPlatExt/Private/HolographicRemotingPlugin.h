#pragma once

// Currently remoting only supports x64 Windows: Editor and Packaged Exe
#define SUPPORTS_REMOTING (PLATFORM_WINDOWS && PLATFORM_64BITS)

#if SUPPORTS_REMOTING

#include "OpenXRCommon.h"
#include "openxr_msft_remoting.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

#include "OpenXRCore.h"
#include "HeadMountedDisplayTypes.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "MRPlatExtRuntimeSettings.h"

// Microsoft.Holographic.AppRemoting binaries only exist for Win64.
#define SUPPORTS_REMOTING_IN_PACKAGED_BUILD (!WITH_EDITOR && PLATFORM_DESKTOP && PLATFORM_64BITS)

namespace MRPlatExt
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

	private:
		void* LoaderHandle;
		bool remotingEnabled = false;

		XrInstance Instance;
		XrSystemId System = XR_NULL_SYSTEM_ID;

		RemotingConnectionData remotingConnectionData;

		XrRemotingConnectionStateMSFT GetRemotingConnectionState();
		void ConnectToRemoteDevice(RemotingConnectionData data);
		void DisconnectFromRemoteDevice();

		void SetRemotingStatusText(FString message, FLinearColor statusColor);
		void UpdateDisconnectedText();
		FString GetDisconnectedReason(int index);

		bool ParseRemotingCmdArgs();
	};
}	 // namespace MRPlatExt

#endif
