// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "AzureSpatialAnchorsForOpenXR.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

#include "OpenXRCore.h"
#include "MicrosoftOpenXR.h"

#include "Engine.h"
#include "SpatialAnchorPlugin.h"

using namespace winrt::Microsoft::Azure::SpatialAnchors;

void FAzureSpatialAnchorsForOpenXR::StartupModule()
{
	IModularFeatures::Get().RegisterModularFeature(IAzureSpatialAnchors::GetModularFeatureName(), static_cast<IAzureSpatialAnchors*>(this));

	const FString PluginBaseDir = IPluginManager::Get().FindPlugin("MicrosoftOpenXR")->GetBaseDir();
	FString PackageRelativePath = PluginBaseDir / THIRDPARTY_BINARY_SUBFOLDER;

	// On HoloLens, DLLs must be loaded relative to the package with no ".."'s in the path. 
	// If using FPlatformProcess::PushDLLDirectory, the library path must be made relative to the RootDir.
#if PLATFORM_HOLOLENS
	FPaths::MakePathRelativeTo(PackageRelativePath, *(FPaths::RootDir() + TEXT("/")));
#endif

	const FString CourseRelocDLL = "CoarseRelocUW.dll";
	const FString AsaDLL = "Microsoft.Azure.SpatialAnchors.dll";

	FPlatformProcess::PushDllDirectory(*PackageRelativePath);
	if (!FPlatformProcess::GetDllHandle(*CourseRelocDLL))
	{
		UE_LOG(LogHMD, Warning, TEXT("Dll \'%s\' can't be loaded from \'%s\'"), *CourseRelocDLL, *PackageRelativePath);
	}
	if (!FPlatformProcess::GetDllHandle(*AsaDLL))
	{
		UE_LOG(LogHMD, Warning, TEXT("Dll \'%s\' can't be loaded from \'%s\'"), *AsaDLL, *PackageRelativePath);
	}
	FPlatformProcess::PopDllDirectory(*PackageRelativePath);

	spatialPerceptionAccessStatus = winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus::Unspecified;
	winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus> 
		getPerceptionAccessAsyncOperation = winrt::Windows::Perception::Spatial::SpatialAnchorExporter::RequestAccessAsync();
	
	getPerceptionAccessAsyncOperation.Completed([this](
		winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus> asyncOperation,
		winrt::Windows::Foundation::AsyncStatus status)
	{
		if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
		{
			spatialPerceptionAccessStatus = asyncOperation.GetResults();
		}
	});
}

void FAzureSpatialAnchorsForOpenXR::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(IAzureSpatialAnchors::GetModularFeatureName(), static_cast<IAzureSpatialAnchors*>(this));
}


bool FAzureSpatialAnchorsForOpenXR::CreateSession()
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateSession called"));

	if (m_cloudSession != nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateSession called, but session already exists!  Ignoring."));
		return true;
	}

	m_cloudSession = winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchorSession();

	AddEventListeners();

	return m_cloudSession != nullptr;
}

void FAzureSpatialAnchorsForOpenXR::AddEventListeners()
{
	if (m_cloudSession == nullptr)
	{
		return;
	}

	if (m_anchorLocatedToken)
	{
		// Event listeners have already been setup, possible if we 'configure' multiple times.
		return;
	}

	m_anchorLocatedToken = m_cloudSession.AnchorLocated(winrt::auto_revoke, [this](auto&&, auto&& args)
	{
		UE_LOG(LogHMD, Log,
			TEXT("AnchorLocated watcher %d has Located."),
			args.Watcher().Identifier());

		LocateAnchorStatus Status = args.Status();
		IAzureSpatialAnchors::CloudAnchorID CloudAnchorID = CloudAnchorID_Invalid;

		switch (Status)
		{
		case LocateAnchorStatus::Located:
		{
			winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor loadedCloudAnchor = args.Anchor();
			CloudAnchorID = CloudAnchorIdentifierToID(loadedCloudAnchor.Identifier());
			if (CloudAnchorID == CloudAnchorID_Invalid)
			{
				CloudAnchorID = GetNextCloudAnchorID();
				{
					auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
					m_cloudAnchors.insert(std::make_pair(CloudAnchorID, loadedCloudAnchor));
				}

				UE_LOG(LogHMD, Log,
					TEXT("LocateAnchorStatus::Located Id: %s. Created CloudAnchor %d"),
					args.Anchor().Identifier().c_str(), CloudAnchorID);
			}
			else
			{
				UE_LOG(LogHMD, Log,
					TEXT("LocateAnchorStatus::Located Id: %s. Cloud Anchor already existed."),
					args.Anchor().Identifier().c_str());
			}
		}
		break;
		case LocateAnchorStatus::AlreadyTracked:
		{
			winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor loadedCloudAnchor = args.Anchor();
			CloudAnchorID = CloudAnchorIdentifierToID(loadedCloudAnchor.Identifier());

			UE_LOG(LogHMD, Log,
				TEXT("LocateAnchorStatus::AlreadyTracked CloudAnchorID : %d."),
				CloudAnchorID);

			assert(CloudAnchorID != CloudAnchorID_Invalid);
		}
		break;
		case LocateAnchorStatus::NotLocated:
		{
			// This gets called repeatedly for a while until something else happens.
			UE_LOG(LogHMD, Log, TEXT("LocateAnchorStatus::NotLocated"));

			winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor loadedCloudAnchor = args.Anchor();
			CloudAnchorID = CloudAnchorIdentifierToID(loadedCloudAnchor.Identifier());
			if (CloudAnchorID == CloudAnchorID_Invalid)
			{
				CloudAnchorID = GetNextCloudAnchorID();
				{
					auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
					m_cloudAnchors.insert(std::make_pair(CloudAnchorID, loadedCloudAnchor));
				}

				UE_LOG(LogHMD, Log,
					TEXT("LocateAnchorStatus::NotLocated Id: %s. Created CloudAnchor %d"),
					args.Anchor().Identifier().c_str(), CloudAnchorID);
			}
			else
			{
				UE_LOG(LogHMD, Log,
					TEXT("LocateAnchorStatus::NotLocated Id: %s. Cloud Anchor already existed."),
					args.Anchor().Identifier().c_str());
			}
		}
		break;
		case LocateAnchorStatus::NotLocatedAnchorDoesNotExist:
		{
			UE_LOG(LogHMD, Log,
				TEXT("LocateAnchorStatus::NotLocatedAnchorDoesNotExist."));
		}
		break;
		default:
			assert(false);
		}

		AnchorLocatedCallback(args.Watcher().Identifier(), static_cast<int32>(Status), CloudAnchorID);
	});

	m_locateAnchorsCompletedToken = m_cloudSession.LocateAnchorsCompleted(winrt::auto_revoke, [this](auto&&, auto&& args)
	{
		UE_LOG(LogHMD, Log,
			TEXT("LocateAnchorsCompleted watcher: %d has completed."),
			args.Watcher().Identifier());

		LocateAnchorsCompletedCallback(args.Watcher().Identifier(), args.Cancelled());
	});

	m_sessionUpdatedToken = m_cloudSession.SessionUpdated(winrt::auto_revoke, [this](auto&&, auto&& args)
	{
		winrt::Microsoft::Azure::SpatialAnchors::SessionStatus status = args.Status();
		SessionUpdatedCallback(status.ReadyForCreateProgress(), status.RecommendedForCreateProgress(), status.SessionCreateHash(), status.SessionLocateHash(), static_cast<int32>(status.UserFeedback()));
	});

	m_errorToken = m_cloudSession.Error(winrt::auto_revoke, [this](auto&&, auto&& args)
	{
		UE_LOG(LogHMD, Error,
			TEXT("CloudSession ErrorMessage: %s."),
			args.ErrorMessage().c_str());
	});

	m_onLogDebugToken = m_cloudSession.OnLogDebug(winrt::auto_revoke, [this](auto&&, auto&& args)
	{
		UE_LOG(LogHMD, Log,
			TEXT("CloudSession LogDebug: %s."),
			args.Message().c_str());
	});
}

void FAzureSpatialAnchorsForOpenXR::RemoveEventListeners()
{
	if (m_cloudSession != nullptr)
	{
		m_anchorLocatedToken.revoke();
		m_locateAnchorsCompletedToken.revoke();
		m_sessionUpdatedToken.revoke();
		m_errorToken.revoke();
		m_onLogDebugToken.revoke();
	}
}

IAzureSpatialAnchors::CloudAnchorID FAzureSpatialAnchorsForOpenXR::CloudAnchorIdentifierToID(const winrt::hstring& CloudAnchorIdentifier) const
{
	auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
	for (auto& itr : m_cloudAnchors)
	{
		if (itr.second.Identifier() == CloudAnchorIdentifier)
		{
			return itr.first;
		}
	}
	return CloudAnchorID_Invalid;
}

IAzureSpatialAnchors::CloudAnchorID FAzureSpatialAnchorsForOpenXR::GetNextCloudAnchorID()
{
	// Note: IDs must remain unique across the creation of multiple Interop's in a UE4 app lifetime (important for remoting which can run multiple interops).
	static std::atomic<int> NextCloudAnchorID(0);
	return NextCloudAnchorID++;
}

void FAzureSpatialAnchorsForOpenXR::DestroySession()
{
	FAzureSpatialAnchorsBase::DestroySession();
	
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::DestroySession called"));

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::DestroySession called, but session does not exist!  Ignoring."));
		return;
	}

	//DestroySession
	RemoveEventListeners();
	{
		auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
		m_cloudAnchors.clear();
	}
	m_sessionStarted = false;
	m_cloudSession = nullptr;
}

bool FAzureSpatialAnchorsForOpenXR::CheckForSession(const wchar_t* context) const
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("%s called, but session does not exist!  Ignoring."), context);

		return false;
	}

	if (m_sessionStarted == false)
	{
		UE_LOG(LogHMD, Log, TEXT("%s called, but session has not started.  Ignoring."), context);

		return false;
	}

	return true;
}

void FAzureSpatialAnchorsForOpenXR::GetAccessTokenWithAccountKeyAsync(const FString& AccountKey, Callback_Result_String Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetAccessTokenWithAccountKeyAsync called"));

	if (!CheckForSession(L"GetAccessTokenWithAccountKeyAsync"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, L"", L"");
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> getAccessTokenAsyncOperation =
			m_cloudSession.GetAccessTokenWithAccountKeyAsync(*AccountKey);
		getAccessTokenAsyncOperation.Completed([this, Callback](
			winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> asyncOperation,
			winrt::Windows::Foundation::AsyncStatus status)
		{
			if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				winrt::hstring AccessToken = asyncOperation.GetResults();

				Callback(EAzureSpatialAnchorsResult::Success, L"", AccessToken.c_str());
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log,
			TEXT("GetAccessTokenWithAccountKey_Coroutine failed to get status. message: %s."),
			e.message().c_str());

		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str(), L"");
	}
}

void FAzureSpatialAnchorsForOpenXR::GetAccessTokenWithAuthenticationTokenAsync(const FString& AuthenticationToken, Callback_Result_String Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetAccessTokenWithAuthenticationTokenAsync called"));

	if (!CheckForSession(L"GetAccessTokenWithAuthenticationTokenAsync"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, L"", L"");
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> getAccessTokenAsyncOperation =
			m_cloudSession.GetAccessTokenWithAuthenticationTokenAsync(*AuthenticationToken);
		getAccessTokenAsyncOperation.Completed([this, Callback](
			winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> asyncOperation,
			winrt::Windows::Foundation::AsyncStatus status)
		{
			if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				winrt::hstring AccessToken = asyncOperation.GetResults();

				Callback(EAzureSpatialAnchorsResult::Success, L"", AccessToken.c_str());
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log,
			TEXT("GetAccessTokenWithAuthenticationToken_Coroutine failed to get status. message: %s."),
			e.message().c_str());

		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str(), L"");
	}
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::StartSession()
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StartSession called"));

	if (spatialPerceptionAccessStatus != winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus::Allowed)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StartSession cannot be started yet because spatial perception access cannot be verified."));
		return EAzureSpatialAnchorsResult::NotStarted;
	}

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StartSession called, but session does not exist!  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	if (m_sessionStarted == true)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StartSession called, but session is already started.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailAlreadyStarted;
	}

	m_cloudSession.Start();
	m_sessionStarted = true;

	return EAzureSpatialAnchorsResult::Success;
}

void FAzureSpatialAnchorsForOpenXR::StopSession()
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StopSession called"));

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StopSession called, but session has already been cleaned up.  Ignoring."));
		return;
	}

	if (m_sessionStarted == false)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StopSession called, but session is not started.  Ignoring."));
		return;
	}

	m_sessionStarted = false;
	m_cloudSession.Stop();
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::ResetSession()
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::ResetSession called"));

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::ResetSession called, but session has already been cleaned up.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	m_cloudSession.Reset();
	return EAzureSpatialAnchorsResult::Success;
}
void FAzureSpatialAnchorsForOpenXR::DisposeSession()
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::DisposeSession called"));

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::DisposeSession called, but no session exists.  Ignoring."));
		return;
	}

	m_cloudSession.Dispose();
	m_cloudSession = nullptr;
}

void FAzureSpatialAnchorsForOpenXR::GetSessionStatusAsync(Callback_Result_SessionStatus Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetSessionStatusAsync called"));

	if (!CheckForSession(L"GetSessionStatusAsync"))
	{
		FAzureSpatialAnchorsSessionStatus EmptyStatus;
		Callback(EAzureSpatialAnchorsResult::FailNoSession, L"", EmptyStatus);
		return;
	}

	try
	{
		UE_LOG(LogHMD, Log, TEXT("GetSessionStatus_Coroutine getting status."));

		winrt::Windows::Foundation::IAsyncOperation<SessionStatus> getSessionStatusAsyncOperation =
			m_cloudSession.GetSessionStatusAsync();
		getSessionStatusAsyncOperation.Completed([this, Callback](
			winrt::Windows::Foundation::IAsyncOperation<SessionStatus> asyncOperation,
			winrt::Windows::Foundation::AsyncStatus status)
		{
			if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				SessionStatus NativeStatus = asyncOperation.GetResults();
				FAzureSpatialAnchorsSessionStatus InteropSessionStatus;

				InteropSessionStatus.ReadyForCreateProgress = NativeStatus.ReadyForCreateProgress();
				InteropSessionStatus.RecommendedForCreateProgress = NativeStatus.RecommendedForCreateProgress();
				InteropSessionStatus.SessionCreateHash = NativeStatus.SessionCreateHash();
				InteropSessionStatus.SessionLocateHash = NativeStatus.SessionLocateHash();
				InteropSessionStatus.feedback = static_cast<EAzureSpatialAnchorsSessionUserFeedback>(NativeStatus.UserFeedback());

				UE_LOG(LogHMD, Log, TEXT("GetSessionStatus_Coroutine got status."));

				Callback(EAzureSpatialAnchorsResult::Success, L"", InteropSessionStatus);
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log,
			TEXT("GetSessionStatus_Coroutine failed to get status. message: %s."),
			e.message().c_str());

		FAzureSpatialAnchorsSessionStatus EmptyStatus;
		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str(), EmptyStatus);
	}
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::ConstructAnchor(UARPin* InARPin, CloudAnchorID& OutCloudAnchorID)
{
	if (!InARPin)
	{
		UE_LOG(LogAzureSpatialAnchors, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::ConstructAnchor called with null ARPin.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoARPin;
	}

	if (void* nativeResource = InARPin->GetNativeResource())
	{
		MicrosoftOpenXR::SAnchorMSFT* AnchorMSFT = reinterpret_cast<MicrosoftOpenXR::SAnchorMSFT*>(nativeResource);

		winrt::Windows::Perception::Spatial::SpatialAnchor localAnchor = nullptr;
		if (UMicrosoftOpenXRFunctionLibrary::GetPerceptionAnchorFromOpenXRAnchor((void*)AnchorMSFT->Anchor, (void**)&localAnchor))
		{
			winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor newCloudAnchor;
			newCloudAnchor.LocalAnchor(localAnchor);
			OutCloudAnchorID = GetNextCloudAnchorID();

			auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
			m_cloudAnchors.insert(std::make_pair(OutCloudAnchorID, newCloudAnchor));

			return EAzureSpatialAnchorsResult::Success;
		}
	}

	return EAzureSpatialAnchorsResult::FailNoAnchor;
}

CloudSpatialAnchor* FAzureSpatialAnchorsForOpenXR::GetNativeCloudAnchor(CloudAnchorID cloudAnchorID)
{
	auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
	auto iterator = m_cloudAnchors.find(cloudAnchorID);
	if (iterator == m_cloudAnchors.end())
	{
		return nullptr;
	}
	return &(iterator->second);
}

void FAzureSpatialAnchorsForOpenXR::CreateAnchorAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateAnchorAsync for CloudAnchorID: %d"), InCloudAnchorID);

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateAnchorAsync failed because cloud anchor for CloudAnchorID %d does not exist"), InCloudAnchorID);
	
		Callback(EAzureSpatialAnchorsResult::FailNoCloudAnchor, nullptr);
		return;
	}

	if (!CheckForSession(L"CreateAnchorAsync"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, nullptr);
		return;
	}

	try
	{
		UE_LOG(LogHMD, Log, TEXT("CreateAnchor_Coroutine saving cloud anchor: %d"), InCloudAnchorID);

		winrt::Windows::Foundation::IAsyncAction createAnchorAsyncAction =
			m_cloudSession.CreateAnchorAsync(*cloudAnchor);

		createAnchorAsyncAction.Completed([this, InCloudAnchorID, Callback](
			winrt::Windows::Foundation::IAsyncAction action, winrt::Windows::Foundation::AsyncStatus status)
		{
			if (action.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				UE_LOG(LogHMD, Log, TEXT("CreateAnchor_Coroutine saved cloud anchor [%d]"), InCloudAnchorID);

				UE_LOG(LogHMD, Log, TEXT("CreateAnchor_Coroutine making callback"));
				Callback(EAzureSpatialAnchorsResult::Success, L"");
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("CreateAnchor_Coroutine failed to save cloud anchor [%d] message: %s"), 
			InCloudAnchorID, e.message().c_str());

		UE_LOG(LogHMD, Log, TEXT("CreateAnchor_Coroutine making callback"));
		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str());
	}
}

void FAzureSpatialAnchorsForOpenXR::DeleteAnchorAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::DeleteAnchorAsync for CloudAnchorID %d"), InCloudAnchorID);

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::DeleteAnchorAsync failed because cloud anchor for CloudAnchorID %d does not exist!"), InCloudAnchorID);

		Callback(EAzureSpatialAnchorsResult::FailNoCloudAnchor, L"");
		return;
	}

	if (!CheckForSession(L"DeleteAnchorAsync"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, L"");
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncAction deleteAnchorAsyncAction =
			m_cloudSession.DeleteAnchorAsync(*cloudAnchor);

		deleteAnchorAsyncAction.Completed([this, &InCloudAnchorID, Callback](
			winrt::Windows::Foundation::IAsyncAction action, winrt::Windows::Foundation::AsyncStatus status)
		{
			if (action.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
				m_cloudAnchors.erase(InCloudAnchorID);

				Callback(EAzureSpatialAnchorsResult::Success, L"");
				UE_LOG(LogHMD, Log, TEXT("DeleteAnchor deleted cloud anchor: %d"), InCloudAnchorID);
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("DeleteAnchorAsync failed to delete cloud anchor %d message:"), InCloudAnchorID, e.message().c_str());

		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str());
	}
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::CreateWatcher(const FAzureSpatialAnchorsLocateCriteria& InLocateCriteria, float InWorldToMetersScale, WatcherID& OutWatcherID, FString& OutErrorString)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateWatcher"));

	if (spatialPerceptionAccessStatus != winrt::Windows::Perception::Spatial::SpatialPerceptionAccessStatus::Allowed)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateWatcher cannot be started yet because spatial perception access cannot be verified."));
		return EAzureSpatialAnchorsResult::NotStarted;
	}

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateWatcher failed because there is no session.  You must create the AzureSpatialAnchors session first."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	AnchorLocateCriteria criteria = AnchorLocateCriteria();

	criteria.BypassCache(InLocateCriteria.bBypassCache);

	UAzureCloudSpatialAnchor::AzureCloudAnchorID NearCloudAnchorID = 
		InLocateCriteria.NearAnchor ? InLocateCriteria.NearAnchor->CloudAnchorID : IAzureSpatialAnchors::CloudAnchorID_Invalid;
	if (NearCloudAnchorID != CloudAnchorID_Invalid)
	{
		winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* sourceAnchor = GetNativeCloudAnchor(NearCloudAnchorID);
		if (!sourceAnchor)
		{
			UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateWatcher failed because cloud anchor with NearCloudAnchorID %d does not exist!"), NearCloudAnchorID);
			return EAzureSpatialAnchorsResult::FailNoCloudAnchor;
		}
		NearAnchorCriteria nearAnchorCriteria;
		nearAnchorCriteria.DistanceInMeters(InLocateCriteria.NearAnchorDistance * InWorldToMetersScale);
		nearAnchorCriteria.MaxResultCount(InLocateCriteria.NearAnchorMaxResultCount);
		nearAnchorCriteria.SourceAnchor(*sourceAnchor);
		criteria.NearAnchor(nearAnchorCriteria);
	}

	if (InLocateCriteria.bSearchNearDevice)
	{
		NearDeviceCriteria nearDeviceCriteria = NearDeviceCriteria();
		nearDeviceCriteria.DistanceInMeters(InLocateCriteria.NearDeviceDistance);
		nearDeviceCriteria.MaxResultCount(InLocateCriteria.NearDeviceMaxResultCount);
		criteria.NearDevice(nearDeviceCriteria);
	}

	if (InLocateCriteria.Identifiers.Num() > 0)
	{
		std::vector<winrt::hstring> Identifiers;
		Identifiers.reserve(InLocateCriteria.Identifiers.Num());
		for (int32_t i = 0; i < InLocateCriteria.Identifiers.Num(); ++i)
		{
			Identifiers.push_back(*InLocateCriteria.Identifiers[i]);
		}
		criteria.Identifiers(Identifiers);
	}

	criteria.RequestedCategories(static_cast<AnchorDataCategory>(InLocateCriteria.RequestedCategories));
	criteria.Strategy(static_cast<LocateStrategy>(InLocateCriteria.Strategy));

	{
		try
		{
			auto watcher = m_cloudSession.CreateWatcher(criteria);
			auto lock = std::unique_lock<std::mutex>(m_watcherMapMutex);
			m_watcherMap.insert({ static_cast<WatcherID>(watcher.Identifier()), watcher });
			OutWatcherID = watcher.Identifier();
			UE_LOG(LogHMD, Log, TEXT("CreateWatcher created watcher %d"),watcher.Identifier());
			return EAzureSpatialAnchorsResult::Success;
		}
		catch (const winrt::hresult_error& e)
		{
			OutErrorString = e.message().c_str();
			UE_LOG(LogHMD, Log, TEXT("CreateWatcher failed to create watcher.  message: %s"), e.message().c_str());
			return EAzureSpatialAnchorsResult::FailSeeErrorString;
		}
	}
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetActiveWatchers(TArray<WatcherID>& OutWatcherIDs)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::ResetSession called"));

	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetActiveWatchers called, but session has already been cleaned up.  Returning empty list."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchorWatcher> Watchers = m_cloudSession.GetActiveWatchers();

	OutWatcherIDs.Reset(Watchers.Size());
	int32_t Index = 0;

	for (uint32_t i = 0; i < Watchers.Size(); ++i)
	{
		OutWatcherIDs.Add(Watchers.GetAt(i).Identifier());
	}

	return EAzureSpatialAnchorsResult::Success;
}

void FAzureSpatialAnchorsForOpenXR::GetAnchorPropertiesAsync(const FString& InCloudAnchorIdentifier, Callback_Result_CloudAnchorID Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetAnchorPropertiesAsync for cloud identifier %d"), *InCloudAnchorIdentifier);

	if (InCloudAnchorIdentifier.IsEmpty())
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetAnchorPropertiesAsync failed because cloud anchor Identifier is null or empty!"));
		Callback(EAzureSpatialAnchorsResult::FailBadCloudAnchorIdentifier, L"", CloudAnchorID_Invalid);
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncOperation<CloudSpatialAnchor> getAnchorPropertiesAsyncOperation =
			m_cloudSession.GetAnchorPropertiesAsync(*InCloudAnchorIdentifier);

		getAnchorPropertiesAsyncOperation.Completed([this, Callback, &InCloudAnchorIdentifier](
			winrt::Windows::Foundation::IAsyncOperation<CloudSpatialAnchor> asyncOperation,
			winrt::Windows::Foundation::AsyncStatus status)
		{
			if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				CloudSpatialAnchor FoundCloudAnchor = asyncOperation.GetResults();

				// If we already have this CloudAnchor return its ID.
				CloudAnchorID CloudAnchorID = CloudAnchorIdentifierToID(FoundCloudAnchor.Identifier());
				if (CloudAnchorID == CloudAnchorID_Invalid)
				{
					CloudAnchorID = GetNextCloudAnchorID();
					{
						auto lock = std::unique_lock<std::mutex>(m_cloudAnchorsMutex);
						m_cloudAnchors.insert(std::make_pair(CloudAnchorID, FoundCloudAnchor));
					}
				}

				UE_LOG(LogHMD, Log, TEXT("GetAnchorProperties found anchor: %d with identifier: %s"), CloudAnchorID, *InCloudAnchorIdentifier);
				Callback(EAzureSpatialAnchorsResult::Success, L"", CloudAnchorID);
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("GetAnchorProperties failed to find cloud anchor with identifier %s, message: %s"), *InCloudAnchorIdentifier, e.message().c_str());
		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str(), CloudAnchorID_Invalid);
	}
}

void FAzureSpatialAnchorsForOpenXR::RefreshAnchorPropertiesAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::RefreshCloudAnchorProperties for cloud anchor %d"), InCloudAnchorID);

	CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::RefreshCloudAnchorProperties failed because cloud anchor %d does not exist!  You must create the cloud anchor first."), InCloudAnchorID);

		Callback(EAzureSpatialAnchorsResult::FailNoAnchor, nullptr);
		return;
	}

	if (!CheckForSession(L"RefreshCloudAnchorProperties"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, nullptr);
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncAction refreshAnchorPropertiesAction =
			m_cloudSession.RefreshAnchorPropertiesAsync(*cloudAnchor);
		refreshAnchorPropertiesAction.Completed([this, Callback, &InCloudAnchorID](
			winrt::Windows::Foundation::IAsyncAction action, winrt::Windows::Foundation::AsyncStatus status)
		{
			if (action.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				UE_LOG(LogHMD, Log, TEXT("RefreshCloudAnchorProperties refreshed cloud anchor %d"), InCloudAnchorID);
				Callback(EAzureSpatialAnchorsResult::Success, L"");
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("RefreshCloudAnchorProperties failed to refresh cloud anchor: %d, message: %s"), InCloudAnchorID, e.message().c_str());
		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str());
	}
}
void FAzureSpatialAnchorsForOpenXR::UpdateAnchorPropertiesAsync(CloudAnchorID InCloudAnchorID, Callback_Result Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::UpdateCloudAnchorProperties for cloud anchor %d"), InCloudAnchorID);

	CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::UpdateCloudAnchorProperties failed because cloud anchor %d does not exist!  You must create the cloud anchor first."), InCloudAnchorID);

		Callback(EAzureSpatialAnchorsResult::FailNoAnchor, nullptr);
		return;
	}

	if (!CheckForSession(L"UpdateCloudAnchorProperties"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, nullptr);
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncAction updateAnchorPropertiesAction =
			m_cloudSession.UpdateAnchorPropertiesAsync(*cloudAnchor);

		updateAnchorPropertiesAction.Completed([this, &InCloudAnchorID, Callback](
			winrt::Windows::Foundation::IAsyncAction action, winrt::Windows::Foundation::AsyncStatus status)
		{
			if (action.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				UE_LOG(LogHMD, Log, TEXT("UpdateCloudAnchorProperties updated cloud anchor %d"), InCloudAnchorID);
				Callback(EAzureSpatialAnchorsResult::Success, L"");
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("UpdateCloudAnchorProperties failed to update cloud anchor: %d, message: %s"), InCloudAnchorID, e.message().c_str());
		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str());
	}
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetConfiguration(FAzureSpatialAnchorsSessionConfiguration& OutConfig)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetConfiguration called, but no session exists.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	const winrt::Microsoft::Azure::SpatialAnchors::SessionConfiguration& Config = m_cloudSession.Configuration();
	OutConfig.AccessToken = Config.AccessToken().c_str();
	OutConfig.AccountDomain = Config.AccountDomain().c_str();
	OutConfig.AccountId = Config.AccountId().c_str();
	OutConfig.AccountKey = Config.AccountKey().c_str();
	OutConfig.AuthenticationToken = Config.AuthenticationToken().c_str();
	return EAzureSpatialAnchorsResult::Success;
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::SetConfiguration(const FAzureSpatialAnchorsSessionConfiguration& InConfig)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetConfiguration called, but no session exists.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetConfiguration called"));

	const winrt::Microsoft::Azure::SpatialAnchors::SessionConfiguration& Config = m_cloudSession.Configuration();
	if (!InConfig.AccessToken.IsEmpty()) Config.AccessToken(*InConfig.AccessToken.TrimStartAndEnd());
	if (!InConfig.AccountDomain.IsEmpty()) Config.AccountDomain(*InConfig.AccountDomain.TrimStartAndEnd());
	if (!InConfig.AccountId.IsEmpty()) Config.AccountId(*InConfig.AccountId.TrimStartAndEnd());
	if (!InConfig.AccountKey.IsEmpty()) Config.AccountKey(*InConfig.AccountKey.TrimStartAndEnd());
	if (!InConfig.AuthenticationToken.IsEmpty()) Config.AuthenticationToken(*InConfig.AuthenticationToken.TrimStartAndEnd());
	return EAzureSpatialAnchorsResult::Success;
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::SetLocationProvider(const FCoarseLocalizationSettings& InConfig)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetLocationProvider called, but no session exists.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetLocationProvider called"));

	// Do Coarse Localization Setup
	if (InConfig.bEnable)
	{
		// Create the sensor fingerprint provider
		PlatformLocationProvider sensorProvider = PlatformLocationProvider();
		SensorCapabilities sensors = sensorProvider.Sensors();

		// Allow GPS
		sensors.GeoLocationEnabled(InConfig.bEnableGPS);

		// Allow WiFi scanning
		// Note :if wifi scanning is enabled when remoting an exception will fire soon after session start, but it will be handled.  Localization does work, but perhaps wifi scanning is not working.
		sensors.WifiEnabled(InConfig.bEnableWifi);

		// Bluetooth beacons
		if (InConfig.BLEBeaconUUIDs.Num() > 0)
		{
			// Populate the set of known BLE beacons' UUIDs
			std::vector<winrt::hstring> uuids;
			uuids.reserve(InConfig.BLEBeaconUUIDs.Num());
			for (int32_t i = 0; i < InConfig.BLEBeaconUUIDs.Num(); i++)
			{
				uuids[i] = *(InConfig.BLEBeaconUUIDs[i]);
			}

			// Allow the set of known BLE beacons
			sensors.BluetoothEnabled(true);
			sensors.KnownBeaconProximityUuids(uuids);
		}

		// Set the session's sensor fingerprint provider
		m_cloudSession.LocationProvider(sensorProvider);
	}

	return EAzureSpatialAnchorsResult::Success;
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetLogLevel(EAzureSpatialAnchorsLogVerbosity& OutLogVerbosity)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetLogLevel called, but no session exists.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	OutLogVerbosity = static_cast<EAzureSpatialAnchorsLogVerbosity>(m_cloudSession.LogLevel());
	return EAzureSpatialAnchorsResult::Success;
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::SetLogLevel(EAzureSpatialAnchorsLogVerbosity InLogVerbosity)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetLogLevel called, but no session exists.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetLogLevel called"));

	SessionLogLevel logLevel = (SessionLogLevel)InLogVerbosity;
	if ((logLevel < SessionLogLevel::None) || (logLevel > SessionLogLevel::All))
	{
		logLevel = (logLevel < SessionLogLevel::None) ? SessionLogLevel::None : SessionLogLevel::All;
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetLogLevel called with invalid log level %d.  Clamping the value to "), InLogVerbosity, (int)logLevel);
	}

	m_cloudSession.LogLevel(logLevel);

	return EAzureSpatialAnchorsResult::Success;
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetSessionId(FString& OutSessionID)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetSessionId called, but no session exists.  Returning empty string."));
		OutSessionID = L"";
		return EAzureSpatialAnchorsResult::FailNoSession;
	}
	OutSessionID = m_cloudSession.SessionId().c_str();
	return EAzureSpatialAnchorsResult::Success;
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::StopWatcher(WatcherID WatcherID)
{
	auto lock = std::unique_lock<std::mutex>(m_watcherMapMutex);
	auto itr = m_watcherMap.find(WatcherID);
	if (itr == m_watcherMap.end())
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StopWatcher watcher: %d does not exist!  Ignoring."), WatcherID);
		return EAzureSpatialAnchorsResult::FailNoWatcher;
	}
	else
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::StopWatcher stop watcher: %d"), WatcherID);
		winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchorWatcher& watcher = itr->second;
		watcher.Stop();
		return EAzureSpatialAnchorsResult::Success;
	}
}


EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetCloudSpatialAnchorIdentifier(CloudAnchorID InCloudAnchorID, FString& OutCloudAnchorIdentifier)
{
	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* CloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (CloudAnchor)
	{
		OutCloudAnchorIdentifier = CloudAnchor->Identifier().c_str();
		return EAzureSpatialAnchorsResult::Success;
	}
	else
	{
		OutCloudAnchorIdentifier = L"";
		return EAzureSpatialAnchorsResult::FailAnchorDoesNotExist;
	}
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::SetCloudAnchorExpiration(CloudAnchorID InCloudAnchorID, float InLifetimeInSeconds)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetCloudAnchorExpiration for anchor %d"), InCloudAnchorID);

	if (InLifetimeInSeconds <= 0.0f)
	{
		UE_LOG(LogHMD, Log, TEXT("Warning: FAzureSpatialAnchorsForOpenXR::SetCloudAnchorExpiration setting with lifetime %d which is invalid!  Expiration not set."), InLifetimeInSeconds);
		return EAzureSpatialAnchorsResult::FailBadLifetime;
	}
	int64 lifetimeInt = static_cast<int64>(std::ceil(InLifetimeInSeconds));
	const winrt::Windows::Foundation::TimeSpan future{ std::chrono::seconds{ lifetimeInt } };
	const winrt::Windows::Foundation::DateTime expiration = winrt::clock::now() + future;

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetCloudAnchorExpiration failed because cloudAnchorID %d does not exist!  You must create the cloud anchor first."), InCloudAnchorID);
		return EAzureSpatialAnchorsResult::FailNoCloudAnchor;
	}
	else
	{
		cloudAnchor->Expiration(expiration);
		return EAzureSpatialAnchorsResult::Success;
	}
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetCloudAnchorExpiration(CloudAnchorID InCloudAnchorID, float& OutLifetimeInSeconds)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetCloudAnchorExpiration for anchor %d"), InCloudAnchorID);

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetCloudAnchorExpiration failed because cloudAnchorID %d does not exist!  You must create the cloud anchor first."), InCloudAnchorID);
		return EAzureSpatialAnchorsResult::FailNoCloudAnchor;
	}
	else
	{
		const winrt::Windows::Foundation::TimeSpan lifetimeSpan = cloudAnchor->Expiration() - winrt::clock::now();
		typedef std::chrono::duration<float> floatseconds; // +- about 30 years is representable
		floatseconds seconds = std::chrono::duration_cast<floatseconds>(lifetimeSpan);
		OutLifetimeInSeconds = seconds.count();
		return EAzureSpatialAnchorsResult::Success;
	}
}

EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::SetCloudAnchorAppProperties(CloudAnchorID InCloudAnchorID, const TMap<FString, FString>& InAppProperties)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetCloudAnchorAppProperties for anchor %d"), InCloudAnchorID);

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetCloudAnchorAppProperties failed because cloudAnchorID %d does not exist!  You must create the cloud anchor first."), InCloudAnchorID);
		return EAzureSpatialAnchorsResult::FailNoCloudAnchor;
	}
	else
	{
		auto Properties = cloudAnchor->AppProperties();
		Properties.Clear();
		for (const auto& Pair : InAppProperties)
		{
			Properties.Insert(*Pair.Key, *Pair.Value);
		}

		return EAzureSpatialAnchorsResult::Success;
	}
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::GetCloudAnchorAppProperties(CloudAnchorID InCloudAnchorID, TMap<FString, FString>& OutAppProperties)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetCloudAnchorAppProperties for anchor %d"), InCloudAnchorID);

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InCloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::GetCloudAnchorAppProperties failed because cloudAnchorID %d does not exist!  You must create the cloud anchor first."), InCloudAnchorID);
		return EAzureSpatialAnchorsResult::FailNoCloudAnchor;
	}
	else
	{
		OutAppProperties.Reset();
		for (auto itr : cloudAnchor->AppProperties())
		{
			OutAppProperties.Add(itr.Key().c_str(), itr.Value().c_str());
		}

		return EAzureSpatialAnchorsResult::Success;
	}
}
EAzureSpatialAnchorsResult FAzureSpatialAnchorsForOpenXR::SetDiagnosticsConfig(FAzureSpatialAnchorsDiagnosticsConfig& InConfig)
{
	if (m_cloudSession == nullptr)
	{
		UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetDiagnosticsConfig called, but no session exists.  Ignoring."));
		return EAzureSpatialAnchorsResult::FailNoSession;
	}

	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SetDiagnosticsConfig"));

	const winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchorSessionDiagnostics& Diagnostics = m_cloudSession.Diagnostics();
	Diagnostics.ImagesEnabled(InConfig.bImagesEnabled);
	Diagnostics.LogDirectory(*InConfig.LogDirectory);
	Diagnostics.LogLevel(static_cast<winrt::Microsoft::Azure::SpatialAnchors::SessionLogLevel>(InConfig.LogLevel));
	Diagnostics.MaxDiskSizeInMB(InConfig.MaxDiskSizeInMB);
	return EAzureSpatialAnchorsResult::Success;
}
void FAzureSpatialAnchorsForOpenXR::CreateDiagnosticsManifestAsync(const FString& Description, Callback_Result_String Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::CreateDiagnosticsManifestAsync"))

	if (!CheckForSession(L"FAzureSpatialAnchorsForOpenXR::CreateDiagnosticsManifestAsync"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, L"", L"");
		return;
	}

	try
	{
		winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> createManifestAsyncOperation =
			m_cloudSession.Diagnostics().CreateManifestAsync(*Description);
		createManifestAsyncOperation.Completed([this, Callback](
			winrt::Windows::Foundation::IAsyncOperation<winrt::hstring> asyncOperation,
			winrt::Windows::Foundation::AsyncStatus status)
		{
			if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				winrt::hstring ReturnedString = asyncOperation.GetResults();
				Callback(EAzureSpatialAnchorsResult::Success, L"", ReturnedString.c_str());
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("GetAccessTokenWithAccountKey_Coroutine failed to get status. message: "), e.message().c_str());

		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str(), L"");
	}
}
void FAzureSpatialAnchorsForOpenXR::SubmitDiagnosticsManifestAsync(const FString& ManifestPath, Callback_Result Callback)
{
	UE_LOG(LogHMD, Log, TEXT("FAzureSpatialAnchorsForOpenXR::SubmitDiagnosticsManifestAsync called"));

	if (!CheckForSession(L"SubmitDiagnosticsManifestAsync"))
	{
		Callback(EAzureSpatialAnchorsResult::FailNoSession, L"");
		return;
	}

	EAzureSpatialAnchorsResult Result = EAzureSpatialAnchorsResult::NotStarted;
	const wchar_t* Error = nullptr;
	try
	{
		winrt::Windows::Foundation::IAsyncAction submitManifestAsyncAction =
			m_cloudSession.Diagnostics().SubmitManifestAsync(*ManifestPath);

		submitManifestAsyncAction.Completed([this, Callback](
			winrt::Windows::Foundation::IAsyncAction action, winrt::Windows::Foundation::AsyncStatus status)
		{
			if (action.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				Callback(EAzureSpatialAnchorsResult::Success, L"");
			}
		});
	}
	catch (const winrt::hresult_error& e)
	{
		UE_LOG(LogHMD, Log, TEXT("SubmitDiagnosticsManifest_Coroutine failed to get status. message: %s"), e.message().c_str());
		Callback(EAzureSpatialAnchorsResult::FailSeeErrorString, e.message().c_str());
	}
}

void FAzureSpatialAnchorsForOpenXR::CreateNamedARPinAroundAnchor(const FString& InLocalAnchorId, UARPin*& OutARPin)
{
	TMap<FName, UARPin*> LoadedPins = UARBlueprintLibrary::LoadARPinsFromLocalStore();
	for (auto Pin : LoadedPins)
	{
		if (Pin.Key.ToString().ToLower() == InLocalAnchorId.ToLower())
		{
			OutARPin = Pin.Value;
			return;
		}
	}

	UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateNamedARPinAroundAnchor failed because specified anchor: %s does not exist."), *InLocalAnchorId);
}

bool FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor(const FString& InPinId, UAzureCloudSpatialAnchor* InAzureCloudSpatialAnchor, UARPin*& OutARPin)
{
	if (InAzureCloudSpatialAnchor == nullptr)
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor called with null InAzureCloudSpatialAnchor.  Ignoring."));
		return false;
	}

	if (InAzureCloudSpatialAnchor->ARPin != nullptr)
	{
		OutARPin = InAzureCloudSpatialAnchor->ARPin;
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor called with an InAzureCloudSpatialAnchor that already has an ARPin.  Ignoring."));
		return false;
	}

	if (InPinId.IsEmpty())
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor called with an empty PinId.  Ignoring."));
		return false;
	}

	FName PinIdName(InPinId);

	if (PinIdName == NAME_None)
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor called with illegal PinId of 'None'.  This is dangerous because empty strings cast to Name 'None'."));
	}

	winrt::Microsoft::Azure::SpatialAnchors::CloudSpatialAnchor* cloudAnchor = GetNativeCloudAnchor(InAzureCloudSpatialAnchor->CloudAnchorID);
	if (!cloudAnchor)
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor failed because cloud anchor %d does not exist! You must create the cloud anchor first."), InAzureCloudSpatialAnchor->CloudAnchorID);
		return false;
	}

	winrt::Windows::Perception::Spatial::SpatialAnchor localAnchor = cloudAnchor->LocalAnchor();
	if (!localAnchor)
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor failed because cloud anchor %d does not have a local anchor!  Perhaps it has not localized yet."), InAzureCloudSpatialAnchor->CloudAnchorID);
		return false;
	}

	// First store the perception anchor in the anchor store.
	// If a conflicting local anchor exists, remove it first.
	UARBlueprintLibrary::RemoveARPinFromLocalStore(FName(InPinId.ToLower()));

	if (!UMicrosoftOpenXRFunctionLibrary::StorePerceptionAnchor(InPinId.ToLower(), *(::IUnknown**)&localAnchor))
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor failed to store anchor in local store."));
		return false;
	}

	// Now that the perception anchor is stored in the anchor store, Loading the pin will register the ARPin with the OpenXRAR system.
	TMap<FName, UARPin*> LoadedPins = UARBlueprintLibrary::LoadARPinsFromLocalStore();
	for (auto Pin : LoadedPins)
	{
		if (Pin.Key.ToString().ToLower() == InPinId.ToLower())
		{
			OutARPin = Pin.Value;

			InAzureCloudSpatialAnchor->ARPin = OutARPin;
			break;
		}
	}

	// Since this is a dummy ARPin, stop persisting it so future app launches don't load unnecessary pins.
	UARBlueprintLibrary::RemoveARPinFromLocalStore(FName(InPinId.ToLower()));

	if (OutARPin == nullptr)
	{
		UE_LOG(LogHMD, Warning, TEXT("FAzureSpatialAnchorsForOpenXR::CreateARPinAroundAzureCloudSpatialAnchor failed to find anchor in local store."));
		return false;
	}

	return true;
}

IMPLEMENT_MODULE(FAzureSpatialAnchorsForOpenXR, AzureSpatialAnchorsForOpenXR)

#endif
