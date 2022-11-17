// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "OpenXRCommon.h"
#include "OpenXRCore.h"
#include "MicrosoftOpenXR.h"
#include "HeadMountedDisplayTypes.h"

#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/InputSettings.h"

#include "UObject/NameTypes.h"
#include "UObject/UObjectGlobals.h"
#include "Async/Async.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <unknwn.h>
#include <winrt/Windows.Media.SpeechRecognition.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

#if SUPPORTS_REMOTING
#include "openxr_msft_holographic_remoting.h "
#include "openxr_msft_remoting_speech.h"
#endif

namespace MicrosoftOpenXR
{
	class FSpeechPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void Register();
		void Unregister();

		bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		const void* OnBeginSession(XrSession InSession, const void* InNext) override;
		void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
		void OnEndPlay();

		void AddKeywords(TArray<FKeywordInput> KeywordsToAdd);
		void RemoveKeywords(TArray<FString> KeywordsToRemove);

	private:
		winrt::Windows::Media::SpeechRecognition::SpeechRecognizer SpeechRecognizer = nullptr;
		winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Media::SpeechRecognition::SpeechRecognitionCompilationResult> CompileConstraintsAsyncOperation;
		winrt::Windows::Foundation::IAsyncAction SessionStartAction;
		winrt::event_token ResultsGeneratedToken;
		std::vector<winrt::hstring> Keywords;
		TMap<FString, FKey> KeywordMap;

		APlayerController* GetPlayerController();

		void RegisterKeyword(FKey Key, FString Keyword);
		void CallSpeechCallback(FKey InKey);
		void StartSpeechRecognizer();
		void StopSpeechRecognizer();

		XrSession Session;

		// Remoting
#if SUPPORTS_REMOTING
		PFN_xrInitializeRemotingSpeechMSFT xrInitializeRemotingSpeechMSFT;
		PFN_xrRetrieveRemotingSpeechRecognizedTextMSFT xrRetrieveRemotingSpeechRecognizedTextMSFT;
#endif
		bool bIsRemotingSpeechExtensionEnabled = false;

		void RegisterSpeechCommandsWithRemoting();
	};
}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
