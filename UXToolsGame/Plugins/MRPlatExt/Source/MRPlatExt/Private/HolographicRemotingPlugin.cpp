#include "HolographicRemotingPlugin.h"

#if SUPPORTS_REMOTING

#include "InputCoreTypes.h"

namespace MRPlatExt
{
	void FHolographicRemotingPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FHolographicRemotingPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);

		if (UMRPlatExtRuntimeSettings::Get())
		{
			UMRPlatExtRuntimeSettings::Get()->OnRemotingConnect.Unbind();
			UMRPlatExtRuntimeSettings::Get()->OnRemotingDisconnect.Unbind();
		}
	}

	bool FHolographicRemotingPlugin::GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr)
	{
		remotingEnabled = ParseRemotingCmdArgs();

		if (remotingEnabled)
		{
			FString platformName = FPlatformProperties::IniPlatformName();
			const FString RemotingJsonPath = FPaths::ProjectPluginsDir() / "MRPlatExt/ThirdParty/HolographicAppRemoting" /
				platformName / "Win64/RemotingXR.json";

			if (FPaths::FileExists(RemotingJsonPath))
			{
				// Set the XR_RUNTIME_JSON environment variable to point to the app remoting dll.
				// This must be done before loading openxr_loader.dll.
				SetEnvironmentVariableW(L"XR_RUNTIME_JSON", *RemotingJsonPath);
			}
			else
			{
				// RemotingXR.json could not be found.  Do not attempt to use remoting.
				remotingEnabled = false;
				UE_LOG(LogHMD, Warning,
					TEXT("Remoting is desired, but RemotingXR.json could not be found at expected location: %s"),
					*RemotingJsonPath);
			}
		}

		// Return false to use the intended OpenXR loader.
		return false;
	}

	bool FHolographicRemotingPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		if (remotingEnabled)
		{
			OutExtensions.Add(XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME);
		}

		return true;
	}

	void FHolographicRemotingPlugin::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
	{
		switch ((XrRemotingStructureType)InHeader->type)
		{
		case XR_TYPE_REMOTING_EVENT_DATA_CONNECTED_MSFT:
			SetRemotingStatusText(FString("Connected"), FLinearColor::Green);
			break;
		case XR_TYPE_REMOTING_EVENT_DATA_DISCONNECTED_MSFT:
			UpdateDisconnectedText();
			break;
		}
	}

	// Attempt to create a connection between getting the system and the OpenXRHMD constructor.
	// The OpenXRHMD ctor calls some OpenXR API's which will return default values for HoloLens if a connection has not been made.
	// When connecting to a VR HMD, using default values will be fatal.
	void FHolographicRemotingPlugin::PostGetSystem(XrInstance InInstance, XrSystemId InSystem)
	{
		Instance = InInstance;
		System = InSystem;

		// An app remoting player must be running on the remote device for this call to succeed.
		// Otherwise a disconnect will fire and a connection can happen in the editor at runtime,
		// but default HoloLens values will be used in the remoting runtime.
		//
		// When connecting from a packaged exe, the app remoting player must be running at this time.
		// Otherwise there is no suitable fallback for connecting, since xrLocateSpace will be called
		// before any other event gets a chance to make another connection, and will crash without a valid connection.
		ConnectToRemoteDevice(remotingConnectionData);
	}

	// Bind delegates here, since UMRPlatExtRuntimeSettings will now be initialized.
	const void* FHolographicRemotingPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
#if WITH_EDITOR
		if (UMRPlatExtRuntimeSettings::Get())
		{
			UMRPlatExtRuntimeSettings::Get()->OnRemotingConnect.BindSP(this, &FHolographicRemotingPlugin::ConnectToRemoteDevice);
			UMRPlatExtRuntimeSettings::Get()->OnRemotingDisconnect.BindSP(
				this, &FHolographicRemotingPlugin::DisconnectFromRemoteDevice);
		}
#endif

		return InNext;
	}

	XrRemotingConnectionStateMSFT FHolographicRemotingPlugin::GetRemotingConnectionState()
	{
		if (!remotingEnabled 
			|| System == XR_NULL_SYSTEM_ID)
		{
			return XrRemotingConnectionStateMSFT::XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT;
		}

		PFN_xrRemotingGetConnectionStateMSFT getConnectionState;
		if (XR_FAILED(
			xrGetInstanceProcAddr(Instance, "xrRemotingGetConnectionStateMSFT", (PFN_xrVoidFunction*)&getConnectionState)))
		{
			return XrRemotingConnectionStateMSFT::XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT;
		}

		XrRemotingConnectionStateMSFT connectionState;
		if (XR_FAILED(getConnectionState(Instance, System, &connectionState, nullptr)))
		{
			return XrRemotingConnectionStateMSFT::XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT;
		}

		return connectionState;
	}

	void FHolographicRemotingPlugin::ConnectToRemoteDevice(RemotingConnectionData data)
	{
		if (!remotingEnabled)
		{
			return;
		}

		if (GetRemotingConnectionState() !=
			XrRemotingConnectionStateMSFT::XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT)
		{
			// Already connected.
			return;
		}

		SetRemotingStatusText("Connecting...", FLinearColor::Yellow);
		if (data.IP.IsEmpty())
		{
			SetRemotingStatusText("Cancelled, IP is not set.", FLinearColor::Red);
			return;
		}

		PFN_xrRemotingSetContextPropertiesMSFT setContextProperties;
		XR_ENSURE(
			xrGetInstanceProcAddr(Instance, "xrRemotingSetContextPropertiesMSFT", (PFN_xrVoidFunction*)&setContextProperties));

		// Set remoting properties - this must be done while disconnected.
		XrRemotingRemoteContextPropertiesMSFT remoteContext;
		remoteContext.type = (XrStructureType)XR_TYPE_REMOTING_REMOTE_CONTEXT_PROPERTIES_MSFT;
		remoteContext.next = nullptr;
		remoteContext.enableAudio = data.EnableAudio;
		// Protect against negative or unsuitably low bitrates.
		const int minBitrate = 1000;
		if (data.Bitrate < minBitrate)
		{
			data.Bitrate = minBitrate;
		}
		remoteContext.maxBitrateKbps = data.Bitrate;
		remoteContext.videoCodec = (XrRemotingVideoCodecMSFT)data.ConnectionCodec;
		if (XR_FAILED(setContextProperties(Instance, System, &remoteContext)))
		{
			SetRemotingStatusText("xrRemotingSetContextPropertiesMSFT failed, check remoting inputs.", FLinearColor::Red);
			return;
		}

		// Listen
		if (data.ConnectionType == RemotingConnectionType::Listen)
		{
			XrRemotingListenInfoMSFT listenInfo{ static_cast<XrStructureType>(XR_TYPE_REMOTING_LISTEN_INFO_MSFT) };
			listenInfo.listenInterface = TCHAR_TO_ANSI(*data.IP.TrimStartAndEnd());
			listenInfo.handshakeListenPort = data.Port;
			listenInfo.transportListenPort = data.Port + 1;
			listenInfo.secureConnection = false;

			PFN_xrRemotingListenMSFT remotingListen;
			XR_ENSURE(xrGetInstanceProcAddr(Instance, "xrRemotingListenMSFT", (PFN_xrVoidFunction*)&remotingListen));

			// Connect for remote connection.
			if (XR_FAILED(remotingListen(Instance, System, &listenInfo)))
			{
				SetRemotingStatusText("xrRemotingListenMSFT failed.", FLinearColor::Red);
				return;
			}
		}
		else	// Connect
		{
			XrRemotingConnectInfoMSFT connectInfo{ static_cast<XrStructureType>(XR_TYPE_REMOTING_CONNECT_INFO_MSFT) };
			connectInfo.remoteHostName = TCHAR_TO_ANSI(*data.IP.TrimStartAndEnd());
			connectInfo.remotePort = data.Port;
			connectInfo.secureConnection = false;

			PFN_xrRemotingConnectMSFT remotingConnect;
			XR_ENSURE(xrGetInstanceProcAddr(Instance, "xrRemotingConnectMSFT", (PFN_xrVoidFunction*)&remotingConnect));

			// Connect to remote device.
			if (XR_FAILED(remotingConnect(Instance, System, &connectInfo)))
			{
				SetRemotingStatusText("xrRemotingConnectMSFT failed.", FLinearColor::Red);
				return;
			}
		}
	}

	void FHolographicRemotingPlugin::DisconnectFromRemoteDevice()
	{
		if (!remotingEnabled)
		{
			return;
		}

		if (GetRemotingConnectionState() ==
			XrRemotingConnectionStateMSFT::XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT)
		{
			// Already disconnected.
			return;
		}

		PFN_xrRemotingDisconnectMSFT remotingDisconnect;
		XR_ENSURE(xrGetInstanceProcAddr(Instance, "xrRemotingDisconnectMSFT", (PFN_xrVoidFunction*)&remotingDisconnect));

		// Disconnect from remote device.
		XrRemotingDisconnectInfoMSFT disconnectInfo{ static_cast<XrStructureType>(XR_TYPE_REMOTING_DISCONNECT_INFO_MSFT) };
		if (XR_FAILED(remotingDisconnect(Instance, System, &disconnectInfo)))
		{
			SetRemotingStatusText("xrRemotingDisconnectMSFT failed.", FLinearColor::Red);
			return;
		}
	}

	void FHolographicRemotingPlugin::SetRemotingStatusText(FString message, FLinearColor statusColor)
	{
#if WITH_EDITOR
		if (UMRPlatExtRuntimeSettings::Get() != nullptr)
		{
			UMRPlatExtRuntimeSettings::Get()->OnRemotingStatusChanged.ExecuteIfBound(message, statusColor);
		}
#endif

		UE_LOG(LogHMD, Log, TEXT("HolographicRemotingPlugin::SetRemotingStatusText: %s"), *message);
	}

	void FHolographicRemotingPlugin::UpdateDisconnectedText()
	{
		PFN_xrRemotingGetConnectionStateMSFT getConnectionState;

		FString disconnectText = FString("Disconnected");

		if (System != XR_NULL_SYSTEM_ID && XR_SUCCEEDED(xrGetInstanceProcAddr(Instance, "xrRemotingGetConnectionStateMSFT",
			(PFN_xrVoidFunction*)&getConnectionState)))
		{
			XrRemotingConnectionStateMSFT connectionState;
			XrRemotingDisconnectReasonMSFT disconnectReason;
			if (XR_SUCCEEDED(getConnectionState(Instance, System, &connectionState, &disconnectReason)))
			{
				disconnectText += FString(": ") + GetDisconnectedReason((int)disconnectReason);
			}
		}

		SetRemotingStatusText(disconnectText, FLinearColor::Red);
	}

	FString FHolographicRemotingPlugin::GetDisconnectedReason(int index)
	{
		// Copied from the XrRemotingDisconnectReasonMSFT enum.
		TArray<FString> disconnectedReasons
		{
			FString("XR_REMOTING_DISCONNECT_REASON_NONE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_UNKNOWN_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_NO_SERVER_CERTIFICATE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_PORT_BUSY_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_UNREACHABLE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_CONNECTION_FAILED_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_AUTHENTICATION_FAILED_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_REMOTING_VERSION_MISMATCH_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_INCOMPATIBLE_TRANSPORT_PROTOCOLS_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_FAILED_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_TRANSPORT_PORT_BUSY_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_TRANSPORT_UNREACHABLE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_TRANSPORT_CONNECTION_FAILED_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_PROTOCOL_VERSION_MISMATCH_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_PROTOCOL_ERROR_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_VIDEO_CODEC_NOT_AVAILABLE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_CANCELED_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_CONNECTION_LOST_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_DEVICE_LOST_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_DISCONNECT_REQUEST_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_NETWORK_UNREACHABLE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_CONNECTION_REFUSED_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_VIDEO_FORMAT_NOT_AVAILABLE_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_PEER_DISCONNECT_REQUEST_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_PEER_DISCONNECT_TIMEOUT_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_SESSION_OPEN_TIMEOUT_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_REMOTING_HANDSHAKE_TIMEOUT_MSFT"),
			FString("XR_REMOTING_DISCONNECT_REASON_INTERNAL_ERROR_MSFT")
		};

		if (!disconnectedReasons.IsValidIndex(index))
		{
			return FString("disconnectedReason index out of bounds: ") + FString::FromInt(index);
		}

		return FString::Format(TEXT("{0} ({1})"), { disconnectedReasons[index], index });
	}

	bool FHolographicRemotingPlugin::ParseRemotingCmdArgs()
	{
#if WITH_EDITOR
		GConfig->GetBool(TEXT("/Script/MRPlatExtRuntimeSettings.MRPlatExtRuntimeSettings"), TEXT("bEnableRemotingForEditor"),
			remotingEnabled, GEditorPerProjectIni);

		GConfig->GetString(TEXT("/Script/MRPlatExtRuntimeSettings.MRPlatExtRuntimeSettings"), TEXT("RemoteHoloLensIP"),
			remotingConnectionData.IP, GEditorPerProjectIni);

		GConfig->GetInt(TEXT("/Script/MRPlatExtRuntimeSettings.MRPlatExtRuntimeSettings"), TEXT("MaxBitrate"),
			remotingConnectionData.Bitrate, GEditorPerProjectIni);

		GConfig->GetBool(TEXT("/Script/MRPlatExtRuntimeSettings.MRPlatExtRuntimeSettings"), TEXT("EnableAudio"),
			remotingConnectionData.EnableAudio, GEditorPerProjectIni);

		int connectionType;
		GConfig->GetInt(TEXT("/Script/MRPlatExtRuntimeSettings.MRPlatExtRuntimeSettings"), TEXT("ConnectionType"),
			connectionType, GEditorPerProjectIni);
		remotingConnectionData.ConnectionType = (RemotingConnectionType)connectionType;

		int connectionCodec;
		GConfig->GetInt(TEXT("/Script/MRPlatExtRuntimeSettings.MRPlatExtRuntimeSettings"), TEXT("ConnectionCodec"),
			connectionCodec, GEditorPerProjectIni);
		remotingConnectionData.ConnectionCodec = (RemotingCodec)connectionCodec;

#elif !SUPPORTS_REMOTING_IN_PACKAGED_BUILD
		return false;
#endif

		// Use cmd args for opting into remoting from a packaged exe.
		// These can also be used from the editor, and will supersede values in MRPlatExtRuntimeSettings
		// when connecting from OnGetSystem.  Any connections at runtime will use the editor settings.
		TArray<FString> tokens, switches;
		FCommandLine::Parse(FCommandLine::Get(), tokens, switches);

		remotingConnectionData.EnableAudio =
			switches.Contains(FString("EnableAudio"));

		remotingConnectionData.ConnectionType =
			switches.Contains(FString("Listen")) ? RemotingConnectionType::Listen : RemotingConnectionType::Connect;

		FString codecString;
		FParse::Value(FCommandLine::Get(), TEXT("RemotingCodec="), codecString);
		codecString = codecString.TrimStartAndEnd().ToLower();
		RemotingCodec codec = RemotingCodec::Any;
		if (codecString == FString("h264"))
		{
			codec = RemotingCodec::H264;
		}
		else if (codecString == FString("h265"))
		{
			codec = RemotingCodec::H265;
		}
		else
		{
			codec = RemotingCodec::Any;
		}
		remotingConnectionData.ConnectionCodec = codec;

		FParse::Value(FCommandLine::Get(), TEXT("RemotingBitrate="), remotingConnectionData.Bitrate);

		FString StringToParse;
		if (FParse::Value(FCommandLine::Get(), TEXT("HoloLensRemoting="), StringToParse) &&
			UMRPlatExtRuntimeSettings::ParseAddress(
				StringToParse, remotingConnectionData.IP, remotingConnectionData.Port))
		{
			// An IP to remote to was set, remoting is desired.
			return true;
		}

		return remotingEnabled;
	}

}	 // namespace MRPlatExt

#endif
