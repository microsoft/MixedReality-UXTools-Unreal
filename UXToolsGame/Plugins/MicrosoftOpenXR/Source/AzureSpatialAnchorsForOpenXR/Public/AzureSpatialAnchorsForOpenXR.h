// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "Interfaces/IPluginManager.h"

#include "OpenXRCommon.h"

#include "IAzureSpatialAnchors.h"
#include "AzureSpatialAnchorsBase.h"
#include "AzureCloudSpatialAnchor.h"
#include "UObject/GCObject.h"

#include "HeadMountedDisplayTypes.h"
#include "ARBlueprintLibrary.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <mutex>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.Azure.SpatialAnchors.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

class FAzureSpatialAnchorsForOpenXR : public FAzureSpatialAnchorsBase, public IModuleInterface
{
public:
	void StartupModule() override;

public:
	void ShutdownModule() override;

	// IAzureSpatialAnchors Implementation

	bool CreateSession() override;
	void DestroySession() override;

	void GetAccessTokenWithAccountKeyAsync(const FString& AccountKey, Callback_Result_String Callback) override;
	void GetAccessTokenWithAuthenticationTokenAsync(const FString& AuthenticationToken, Callback_Result_String Callback) override;
	EAzureSpatialAnchorsResult StartSession() override;
	void StopSession() override;
	EAzureSpatialAnchorsResult ResetSession() override;
	void DisposeSession() override;
	void GetSessionStatusAsync(Callback_Result_SessionStatus Callback) override;
	EAzureSpatialAnchorsResult ConstructAnchor(UARPin* InARPin, CloudAnchorID& OutCloudAnchorID) override;
	void CreateAnchorAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback) override;  // note this 'creates' the anchor in the azure cloud, aka saves it to the cloud.
	void DeleteAnchorAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback) override;
	EAzureSpatialAnchorsResult CreateWatcher(const FAzureSpatialAnchorsLocateCriteria& InLocateCriteria, float InWorldToMetersScale, WatcherID& OutWatcherID, FString& OutErrorString) override;
	EAzureSpatialAnchorsResult GetActiveWatchers(TArray<WatcherID>& OutWatcherIDs) override;
	void GetAnchorPropertiesAsync(const FString& InCloudAnchorIdentifier, Callback_Result_CloudAnchorID Callback) override;
	void RefreshAnchorPropertiesAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback) override;
	void UpdateAnchorPropertiesAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback) override;
	EAzureSpatialAnchorsResult GetConfiguration(FAzureSpatialAnchorsSessionConfiguration& OutConfig) override;
	EAzureSpatialAnchorsResult SetConfiguration(const FAzureSpatialAnchorsSessionConfiguration& InConfig) override;
	EAzureSpatialAnchorsResult SetLocationProvider(const FCoarseLocalizationSettings& InConfig) override;
	EAzureSpatialAnchorsResult GetLogLevel(EAzureSpatialAnchorsLogVerbosity& OutLogVerbosity) override;
	EAzureSpatialAnchorsResult SetLogLevel(EAzureSpatialAnchorsLogVerbosity InLogVerbosity) override;
	EAzureSpatialAnchorsResult GetSessionId(FString& OutSessionID) override;

	EAzureSpatialAnchorsResult StopWatcher(WatcherID WatcherID) override;

	EAzureSpatialAnchorsResult GetCloudSpatialAnchorIdentifier(CloudAnchorID InCloudAnchorID, FString& OutCloudAnchorIdentifier) override;
	EAzureSpatialAnchorsResult SetCloudAnchorExpiration(CloudAnchorID InCloudAnchorID, float InLifetimeInSeconds) override;
	EAzureSpatialAnchorsResult GetCloudAnchorExpiration(CloudAnchorID InCloudAnchorID, float& OutLifetimeInSeconds) override;
	EAzureSpatialAnchorsResult SetCloudAnchorAppProperties(CloudAnchorID InCloudAnchorID, const TMap<FString, FString>& InAppProperties) override;
	EAzureSpatialAnchorsResult GetCloudAnchorAppProperties(CloudAnchorID InCloudAnchorID, TMap<FString, FString>& OutAppProperties) override;

	EAzureSpatialAnchorsResult SetDiagnosticsConfig(FAzureSpatialAnchorsDiagnosticsConfig& InConfig) override;
	void CreateDiagnosticsManifestAsync(const FString& Description, Callback_Result_String Callback) override;
	void SubmitDiagnosticsManifestAsync(const FString& ManifestPath, Callback_Result Callback) override;

	void CreateNamedARPinAroundAnchor(const FString& InLocalAnchorId, UARPin*& OutARPin) override;
	bool CreateARPinAroundAzureCloudSpatialAnchor(const FString& InPinId, UAzureCloudSpatialAnchor* InAzureCloudSpatialAnchor, UARPin*& OutARPin) override;

private:
	bool m_sessionStarted{ false };

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchorSession m_cloudSession { nullptr };

	winrt::event_revoker<winrt::Microsoft::Azure::SpatialAnchors::ICloudSpatialAnchorSession> m_anchorLocatedToken;
	winrt::event_revoker<winrt::Microsoft::Azure::SpatialAnchors::ICloudSpatialAnchorSession> m_locateAnchorsCompletedToken;
	winrt::event_revoker<winrt::Microsoft::Azure::SpatialAnchors::ICloudSpatialAnchorSession> m_sessionUpdatedToken;
	winrt::event_revoker<winrt::Microsoft::Azure::SpatialAnchors::ICloudSpatialAnchorSession> m_errorToken;
	winrt::event_revoker<winrt::Microsoft::Azure::SpatialAnchors::ICloudSpatialAnchorSession> m_onLogDebugToken;

	// map of cloud anchors ids to cloud anchors
	std::map<CloudAnchorID, winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor> m_cloudAnchors;
	mutable std::mutex m_cloudAnchorsMutex;

	std::map<WatcherID, winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchorWatcher> m_watcherMap;
	mutable std::mutex m_watcherMapMutex;

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* GetNativeCloudAnchor(CloudAnchorID cloudAnchorID);
	IAzureSpatialAnchors::CloudAnchorID CloudAnchorIdentifierToID(const winrt::hstring& CloudAnchorIdentifier) const;
	IAzureSpatialAnchors::CloudAnchorID GetNextCloudAnchorID();

	void AddEventListeners();
	void RemoveEventListeners();

private:
	bool CheckForSession(const wchar_t* context) const;

	winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus spatialPerceptionAccessStatus;
};
#endif

