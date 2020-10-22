#pragma once

////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation.  All Rights Reserved

#ifdef __cplusplus
extern "C" {
#endif

#define XR_MSFT_holographic_remoting 1
#define XR_MSFT_holographic_remoting_SPEC_VERSION 1
#define XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME "XR_MSFT_holographic_remoting"

	// Extension number: 66 / Enum subrange base: 65000

	// extends XrStructureType
	typedef enum XrRemotingStructureType {
		XR_TYPE_REMOTING_REMOTE_CONTEXT_PROPERTIES_MSFT = 1000065000,
		XR_TYPE_REMOTING_CONNECT_INFO_MSFT = 1000065001,
		XR_TYPE_REMOTING_LISTEN_INFO_MSFT = 1000065002,
		XR_TYPE_REMOTING_DISCONNECT_INFO_MSFT = 1000065003,
		XR_TYPE_REMOTING_EVENT_DATA_LISTENING_MSFT = 1000065004,
		XR_TYPE_REMOTING_EVENT_DATA_CONNECTED_MSFT = 1000065005,
		XR_TYPE_REMOTING_EVENT_DATA_DISCONNECTED_MSFT = 1000065006,
		XR_TYPE_REMOTING_AUTHENTICATION_TOKEN_REQUEST_MSFT = 1000065007,
		XR_TYPE_REMOTING_CERTIFICATE_DATA_MSFT = 1000065008,
		XR_TYPE_REMOTING_CERTIFICATE_VALIDATION_RESULT_MSFT = 1000065009,
		XR_TYPE_REMOTING_SERVER_CERTIFICATE_VALIDATION_MSFT = 1000065010,
		XR_TYPE_REMOTING_AUTHENTICATION_TOKEN_VALIDATION_MSFT = 1000065011,
		XR_TYPE_REMOTING_SERVER_CERTIFICATE_REQUEST_MSFT = 1000065012,
		XR_TYPE_REMOTING_SECURE_CONNECTION_CLIENT_CALLBACKS_MSFT = 1000065013,
		XR_TYPE_REMOTING_SECURE_CONNECTION_SERVER_CALLBACKS_MSFT = 1000065014,
		XR_TYPE_REMOTING_MAX_ENUM = 0x7FFFFFFF
	} XrRemotingStructureType;

	// extends XrResult
	typedef enum XrRemotingResult {
		XR_ERROR_REMOTING_NOT_DISCONNECTED_MSFT = -1000065000,
		XR_ERROR_REMOTING_CODEC_NOT_FOUND_MSFT = -1000065001,
		XR_ERROR_REMOTING_CALLBACK_ERROR_MSFT = -1000065002,
		XR_ERROR_REMOTING_MAX_ENUM = 0x7FFFFFFF
	} XrRemotingResult;

	typedef enum XrRemotingDisconnectReasonMSFT {
		XR_REMOTING_DISCONNECT_REASON_NONE_MSFT = 0,
		XR_REMOTING_DISCONNECT_REASON_UNKNOWN_MSFT = 1,
		XR_REMOTING_DISCONNECT_REASON_NO_SERVER_CERTIFICATE_MSFT = 2,
		XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_PORT_BUSY_MSFT = 3,
		XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_UNREACHABLE_MSFT = 4,
		XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_CONNECTION_FAILED_MSFT = 5,
		XR_REMOTING_DISCONNECT_REASON_AUTHENTICATION_FAILED_MSFT = 6,
		XR_REMOTING_DISCONNECT_REASON_REMOTING_VERSION_MISMATCH_MSFT = 7,
		XR_REMOTING_DISCONNECT_REASON_INCOMPATIBLE_TRANSPORT_PROTOCOLS_MSFT = 8,
		XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_FAILED_MSFT = 9,
		XR_REMOTING_DISCONNECT_REASON_TRANSPORT_PORT_BUSY_MSFT = 10,
		XR_REMOTING_DISCONNECT_REASON_TRANSPORT_UNREACHABLE_MSFT = 11,
		XR_REMOTING_DISCONNECT_REASON_TRANSPORT_CONNECTION_FAILED_MSFT = 12,
		XR_REMOTING_DISCONNECT_REASON_PROTOCOL_VERSION_MISMATCH_MSFT = 13,
		XR_REMOTING_DISCONNECT_REASON_PROTOCOL_ERROR_MSFT = 14,
		XR_REMOTING_DISCONNECT_REASON_VIDEO_CODEC_NOT_AVAILABLE_MSFT = 15,
		XR_REMOTING_DISCONNECT_REASON_CANCELED_MSFT = 16,
		XR_REMOTING_DISCONNECT_REASON_CONNECTION_LOST_MSFT = 17,
		XR_REMOTING_DISCONNECT_REASON_DEVICE_LOST_MSFT = 18,
		XR_REMOTING_DISCONNECT_REASON_DISCONNECT_REQUEST_MSFT = 19,
		XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_NETWORK_UNREACHABLE_MSFT = 20,
		XR_REMOTING_DISCONNECT_REASON_HANDSHAKE_CONNECTION_REFUSED_MSFT = 21,
		XR_REMOTING_DISCONNECT_REASON_VIDEO_FORMAT_NOT_AVAILABLE_MSFT = 22,
		XR_REMOTING_DISCONNECT_REASON_PEER_DISCONNECT_REQUEST_MSFT = 23,
		XR_REMOTING_DISCONNECT_REASON_PEER_DISCONNECT_TIMEOUT_MSFT = 24,
		XR_REMOTING_DISCONNECT_REASON_SESSION_OPEN_TIMEOUT_MSFT = 25,
		XR_REMOTING_DISCONNECT_REASON_REMOTING_HANDSHAKE_TIMEOUT_MSFT = 26,
		XR_REMOTING_DISCONNECT_REASON_INTERNAL_ERROR_MSFT = 27,
		XR_REMOTING_DISCONNECT_REASON_MAX_ENUM = 0x7FFFFFFF
	} XrRemotingDisconnectReasonMSFT;

	typedef enum XrRemotingConnectionStateMSFT {
		XR_REMOTING_CONNECTION_STATE_DISCONNECTED_MSFT = 0,
		XR_REMOTING_CONNECTION_STATE_CONNECTING_MSFT = 1,
		XR_REMOTING_CONNECTION_STATE_CONNECTED_MSFT = 2,
		XR_REMOTING_CONNECTION_STATE_MAX_ENUM = 0x7FFFFFFF
	} XrRemotingConnectionStateMSFT;

	typedef enum XrRemotingVideoCodecMSFT {
		XR_REMOTING_VIDEO_CODEC_ANY = 0,
		XR_REMOTING_VIDEO_CODEC_H264 = 1,
		XR_REMOTING_VIDEO_CODEC_H265 = 2,
		XR_REMOTING_VIDEO_CODEC_MAX_ENUM = 0x7FFFFFFF
	} XrRemotingVideoCodecMSFT;

	typedef enum XrRemotingCertificateNameMismatchMSFT {
		XR_REMOTING_CERTIFICATE_NAME_NOT_CHECKED = 0,
		XR_REMOTING_CERTIFICATE_NAME_MATCH = 1,
		XR_REMOTING_CERTIFICATE_NAME_MISMATCH = 2,
		XR_REMOTING_CERTIFICATE_NAME_MAX_ENUM = 0x7FFFFFFF
	} XrRemotingCertificateNameMismatchMSFT;

	typedef struct XrRemotingRemoteContextPropertiesMSFT {
		XrStructureType type;
		void* next;
		uint32_t maxBitrateKbps;
		XrBool32 enableAudio;
		XrRemotingVideoCodecMSFT videoCodec;
	} XrRemotingRemoteContextPropertiesMSFT;

	typedef struct XrRemotingConnectInfoMSFT {
		XrStructureType type;
		void* next;
		const char* remoteHostName;
		uint16_t remotePort;
		XrBool32 secureConnection;
	} XrRemotingConnectInfoMSFT;

	typedef struct XrRemotingListenInfoMSFT {
		XrStructureType type;
		void* next;
		const char* listenInterface;
		uint16_t handshakeListenPort;
		uint16_t transportListenPort;
		XrBool32 secureConnection;
	} XrRemotingListenInfoMSFT;

	typedef struct XrRemotingDisconnectInfoMSFT {
		XrStructureType type;
		const void* next;
	} XrRemotingDisconnectInfoMSFT;

	typedef struct XrRemotingEventDataListeningMSFT {
		XrStructureType type;
		const void* next;
		uint16_t listeningPort;
	} XrEventDataRemoteContextListeningMSFT;

	typedef struct XrRemotingEventDataConnectedMSFT {
		XrStructureType type;
		const void* next;
	} XrEventDataRemoteContextConnectedMSFT;

	typedef struct XrRemotingEventDataDisconnectedMSFT {
		XrStructureType type;
		const void* next;
		XrRemotingDisconnectReasonMSFT disconnectReason;
	} XrEventDataRemoteContextDisconnectedMSFT;

	typedef struct XrRemotingAuthenticationTokenRequestMSFT {
		XrStructureType type;
		const void* next;
		void* context;
		uint32_t tokenCapacityIn;
		uint32_t tokenSizeOut;
		char* tokenBuffer;
	} XrRemotingAuthenticationTokenRequestMSFT;

	typedef struct XrRemotingCertificateDataMSFT {
		XrStructureType type;
		const void* next;
		uint32_t size;
		const uint8_t* data;
	} XrRemotingCertificateDataMSFT;

	typedef struct XrRemotingCertificateValidationResultMSFT {
		XrStructureType type;
		const void* next;
		XrBool32 trustedRoot;
		XrBool32 revoked;
		XrBool32 expired;
		XrBool32 wrongUsage;
		XrRemotingCertificateNameMismatchMSFT nameMismatch;
		XrBool32 revocationCheckFailed;
		XrBool32 invalidCertOrChain;
	} XrRemotingCertificateValidationResultMSFT;

	typedef struct XrRemotingServerCertificateValidationMSFT {
		XrStructureType type;
		const void* next;
		void* context;
		const char* hostName;
		XrBool32 forceRevocationCheck;
		uint32_t numCertificates;
		const XrRemotingCertificateDataMSFT* certificates;
		XrRemotingCertificateValidationResultMSFT* systemValidationResult;
		XrRemotingCertificateValidationResultMSFT validationResultOut;
	} XrRemotingServerCertificateValidationMSFT;

	typedef struct XrRemotingAuthenticationTokenValidationMSFT {
		XrStructureType type;
		const void* next;
		void* context;
		const char* token;
		XrBool32 tokenValidOut;
	} XrRemotingAuthenticationTokenValidationMSFT;

	typedef struct XrRemotingServerCertificateRequestMSFT {
		XrStructureType type;
		const void* next;
		void* context;
		uint32_t certStoreCapacityIn;
		uint32_t certStoreSizeOut;
		uint8_t* certStoreBuffer;
		uint32_t keyPassphraseCapacityIn;
		uint32_t keyPassphraseSizeOut;
		char* keyPassphraseBuffer;
		uint32_t subjectNameCapacityIn;
		uint32_t subjectNameSizeOut;
		char* subjectNameBuffer;
	} XrRemotingServerCertificateRequestMSFT;

	// Secure connection callback functions (typedef only, no prototype)
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingRequestAuthenticationTokenCallbackMSFT)(XrRemotingAuthenticationTokenRequestMSFT* authenticationTokenRequest);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingValidateServerCertificateCallbackMSFT)(XrRemotingServerCertificateValidationMSFT* serverCertificateValidation);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingValidateAuthenticationTokenCallbackMSFT)(XrRemotingAuthenticationTokenValidationMSFT* authenticationTokenValidation);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingRequestServerCertificateCallbackMSFT)(XrRemotingServerCertificateRequestMSFT* serverCertificateRequest);

	typedef struct XrRemotingSecureConnectionClientCallbacksMSFT {
		XrStructureType type;
		void* next;
		void* context;
		PFN_xrRemotingRequestAuthenticationTokenCallbackMSFT requestAuthenticationTokenCallback;
		PFN_xrRemotingValidateServerCertificateCallbackMSFT validateServerCertificateCallback;
		XrBool32 performSystemValidation;
	} XrRemotingSecureConnectionClientCallbacksMSFT;

	typedef struct XrRemotingSecureConnectionServerCallbacksMSFT {
		XrStructureType type;
		void* next;
		void* context;
		PFN_xrRemotingRequestServerCertificateCallbackMSFT requestServerCertificateCallback;
		PFN_xrRemotingValidateAuthenticationTokenCallbackMSFT validateAuthenticationTokenCallback;
		const char* authenticationRealm;
	} XrRemotingSecureConnectionServerCallbacksMSFT;

	// Remoting extension callable functions
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingSetContextPropertiesMSFT)(XrInstance instance, XrSystemId systemId, const XrRemotingRemoteContextPropertiesMSFT* contextProperties);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingConnectMSFT)(XrInstance instance, XrSystemId systemId, const XrRemotingConnectInfoMSFT* connectInfo);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingListenMSFT)(XrInstance instance, XrSystemId systemId, const XrRemotingListenInfoMSFT* listenInfo);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingDisconnectMSFT)(XrInstance instance, XrSystemId systemId, const XrRemotingDisconnectInfoMSFT* disconnectInfo);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingGetConnectionStateMSFT)(XrInstance instance, XrSystemId systemId, XrRemotingConnectionStateMSFT* connectionState, XrRemotingDisconnectReasonMSFT* lastDisconnectReason);

	typedef XrResult(XRAPI_PTR* PFN_xrRemotingSetSecureConnectionClientCallbacksMSFT)(XrInstance instance, XrSystemId systemId, const XrRemotingSecureConnectionClientCallbacksMSFT* secureConnectionClientCallbacks);
	typedef XrResult(XRAPI_PTR* PFN_xrRemotingSetSecureConnectionServerCallbacksMSFT)(XrInstance instance, XrSystemId systemId, const XrRemotingSecureConnectionServerCallbacksMSFT* secureConnectionServerCallbacks);

#ifndef XR_NO_PROTOTYPES
	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingSetContextPropertiesMSFT(
		XrInstance instance,
		XrSystemId systemId,
		const XrRemotingRemoteContextPropertiesMSFT* contextProperties);

	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingConnectMSFT(
		XrInstance instance,
		XrSystemId systemId,
		const XrRemotingConnectInfoMSFT* connectInfo);

	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingListenMSFT(
		XrInstance instance,
		XrSystemId systemId,
		const XrRemotingListenInfoMSFT* listenInfo);

	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingDisconnectMSFT(
		XrInstance instance,
		XrSystemId systemId,
		const XrRemotingDisconnectInfoMSFT* disconnectInfo);

	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingGetConnectionStateMSFT(
		XrInstance instance,
		XrSystemId systemId,
		XrRemotingConnectionStateMSFT* connectionState,
		XrRemotingDisconnectReasonMSFT* lastDisconnectReason);

	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingSetSecureConnectionClientCallbacksMSFT(
		XrInstance instance,
		XrSystemId systemId,
		const XrRemotingSecureConnectionClientCallbacksMSFT* secureConnectionClientCallbacks);

	XRAPI_ATTR XrResult XRAPI_CALL xrRemotingSetSecureConnectionServerCallbacksMSFT(
		XrInstance instance,
		XrSystemId systemId,
		const XrRemotingSecureConnectionServerCallbacksMSFT* secureConnectionServerCallbacks);

#endif

#ifdef __cplusplus
}
#endif