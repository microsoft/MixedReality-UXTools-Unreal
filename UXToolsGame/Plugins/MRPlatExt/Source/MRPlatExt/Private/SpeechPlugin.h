#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "OpenXRCommon.h"
#include "MRPlatExt.h"
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

namespace MRPlatExt
{
	class FSpeechPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void Register();
		void Unregister();

		void OnStartARSession(class UARSessionConfig* SessionConfig) override;
		void OnStopARSession() override;

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
	};
}	 // namespace MRPlatExt

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
