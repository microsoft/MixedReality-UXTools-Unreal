// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

// Known bug with speech over remoting as of the 2.6.2 remoting NuGet package:
// Adding and removing keywords at runtime is not currently supported.  
// All keywords the app uses when remoting must be declared in the input system.

#include "SpeechPlugin.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "Engine\Engine.h"
#include "GameDelegates.h"

using namespace winrt::Windows::Media::SpeechRecognition;

namespace MicrosoftOpenXR
{
	void FSpeechPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

		FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FSpeechPlugin::OnEndPlay);
	}

	void FSpeechPlugin::Unregister()
	{
		FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	void FSpeechPlugin::OnEndPlay()
	{
		//remove keys from "speech" namespace
		EKeys::RemoveKeysWithCategory(FInputActionSpeechMapping::GetKeyCategory());

		StopSpeechRecognizer();

		Keywords.clear();
		KeywordMap.Empty();
	}

	bool FSpeechPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
#if SUPPORTS_REMOTING
		OutExtensions.Add(XR_MSFT_HOLOGRAPHIC_REMOTING_SPEECH_EXTENSION_NAME);
#endif
		return true;
	}

	const void* FSpeechPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
#if SUPPORTS_REMOTING
		bIsRemotingSpeechExtensionEnabled =
			IOpenXRHMDModule::Get().IsExtensionEnabled(XR_MSFT_HOLOGRAPHIC_REMOTING_SPEECH_EXTENSION_NAME);

		if (bIsRemotingSpeechExtensionEnabled)
		{
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrInitializeRemotingSpeechMSFT", (PFN_xrVoidFunction*)&xrInitializeRemotingSpeechMSFT));
			XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrRetrieveRemotingSpeechRecognizedTextMSFT", (PFN_xrVoidFunction*)&xrRetrieveRemotingSpeechRecognizedTextMSFT));
		}
#endif

		return InNext;
	}

	const void* FSpeechPlugin::OnBeginSession(XrSession InSession, const void* InNext)
	{
		Session = InSession;
		
		// Add all speech keywords from the input system
		const TArray <FInputActionSpeechMapping>& SpeechMappings = GetDefault<UInputSettings>()->GetSpeechMappings();
		for (const FInputActionSpeechMapping& SpeechMapping : SpeechMappings)
		{
			FKey Key(SpeechMapping.GetKeyName());
			FString Keyword = SpeechMapping.GetSpeechKeyword().ToString();

			RegisterKeyword(Key, Keyword);
		}

		if (SpeechMappings.Num() > 0)
		{
			StartSpeechRecognizer();
		}

		return InNext;
	}

	void FSpeechPlugin::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
	{
#if SUPPORTS_REMOTING
		switch ((XrRemotingSpeechStructureType)InHeader->type)
		{
		case XR_TYPE_EVENT_DATA_REMOTING_SPEECH_RECOGNIZED_MSFT:
		{
			auto speechEventData = reinterpret_cast<const XrEventDataRemotingSpeechRecognizedMSFT*>(InHeader);
			std::string text;
			uint32_t dataBytesCount = 0;

			XR_ENSURE_MSFT(xrRetrieveRemotingSpeechRecognizedTextMSFT(InSession, speechEventData->packetId, 0, &dataBytesCount, nullptr));
			text.resize(dataBytesCount);
			XR_ENSURE_MSFT(xrRetrieveRemotingSpeechRecognizedTextMSFT(InSession, speechEventData->packetId, text.size(), &dataBytesCount, text.data()));

			CallSpeechCallback(KeywordMap.FindRef(FString(text.c_str())));
			break;
		}
		case XR_TYPE_EVENT_DATA_REMOTING_SPEECH_RECOGNIZER_STATE_CHANGED_MSFT:
			auto recognizerStateEventData = reinterpret_cast<const XrEventDataRemotingSpeechRecognizerStateChangedMSFT*>(InHeader);
			auto state = recognizerStateEventData->speechRecognizerState;

			if (state == XR_REMOTING_SPEECH_RECOGNIZER_STATE_INITIALIZATION_FAILED_MSFT)
			{
				UE_LOG(LogHMD, Warning, TEXT("Remoting speech recognizer initialization failed."));
				if (strlen(recognizerStateEventData->stateMessage) > 0)
				{
					UE_LOG(LogHMD, Warning, TEXT("Speech recognizer initialization error: %s"), recognizerStateEventData->stateMessage);
				}
			}

			break;
		}
#endif
	}

	void FSpeechPlugin::AddKeywords(TArray<FKeywordInput> KeywordsToAdd)
	{
		if (bIsRemotingSpeechExtensionEnabled)
		{
			UE_LOG(LogHMD, Warning, TEXT("Remoting speech does not currently support adding keywords at runtime."));
			return;
		}

		APlayerController* PlayerController = nullptr;
		UInputSettings* InputSettings = nullptr;
		UInputComponent* InputComponent = nullptr;

		if (KeywordsToAdd.Num() == 0
			|| (PlayerController = GetPlayerController()) == nullptr
			|| (InputSettings = GetMutableDefault<UInputSettings>()) == nullptr
			|| (InputComponent = PlayerController->InputComponent) == nullptr)
		{
			FString WarningText;
			KeywordsToAdd.Num() == 0 ?
				WarningText = FString("FSpeechPlugin::AddKeywords failed: No keywords to register.") :
				WarningText = FString("FSpeechPlugin::AddKeywords failed: Required engine components are null.");

			UE_LOG(LogHMD, Warning, TEXT("%s"), *WarningText);
			return;
		}

		StopSpeechRecognizer();

		for (FKeywordInput InputKeyword : KeywordsToAdd)
		{
			if (InputKeyword.Keyword.IsEmpty() || !InputKeyword.Callback.GetFunctionName().IsValid())
			{
				UE_LOG(LogHMD, Warning, TEXT("Attempting to add an invalid Keyword: %s or Function: %s"), *InputKeyword.Keyword, *InputKeyword.Callback.GetFunctionName().ToString());
				continue;
			}

			FString Keyword = InputKeyword.Keyword;
			// Key name must match FInputActionSpeechMapping::GetKeyName to cleanup correctly.
			FKey Key(FName(*FString::Printf(TEXT("%s_%s"), *FInputActionSpeechMapping::GetKeyCategory().ToString(), *Keyword)));

			// Bind speech delegate to the player's UInputComponent to use the same key events as the input system.
			FInputActionUnifiedDelegate KeywordHandler;
			KeywordHandler.BindDelegate(InputKeyword.Callback.GetUObject(), InputKeyword.Callback.GetFunctionName());

			// Trigger keywords on pressed events.
			FInputActionBinding ActionBinding(Key.GetFName(), IE_Pressed);
			ActionBinding.ActionDelegate = (FInputActionUnifiedDelegate)KeywordHandler;
			
			// Update input system with new binding.
			InputComponent->AddActionBinding(ActionBinding);
			InputSettings->AddActionMapping(FInputActionKeyMapping(Key.GetFName(), Key));

			RegisterKeyword(Key, Keyword);
		}

		StartSpeechRecognizer();
	}

	void FSpeechPlugin::RemoveKeywords(TArray<FString> KeywordsToRemove)
	{
		if (bIsRemotingSpeechExtensionEnabled)
		{
			UE_LOG(LogHMD, Warning, TEXT("Remoting speech does not currently support removing keywords at runtime."));
			return;
		}

		if (KeywordsToRemove.Num() == 0)
		{
			UE_LOG(LogHMD, Warning, TEXT("FSpeechPlugin::RemoveKeywords failed: No keywords to remove."));
			return;
		}

		StopSpeechRecognizer();

		for (FString InputKeyword : KeywordsToRemove)
		{
			InputKeyword = InputKeyword.ToLower();

			// Remove local keyword so it is not included in the next recognizer.
			bool KeywordRemoved = false;
			for (int i = 0; i < Keywords.size(); i++)
			{
				if (Keywords.at(i) == winrt::hstring(*InputKeyword))
				{
					Keywords.erase(Keywords.begin() + i);
					KeywordRemoved = true;
					break;
				}
			}
			if (!KeywordRemoved)
			{
				UE_LOG(LogHMD, Warning, TEXT("FSpeechPlugin::RemoveKeywords failed to remove Keyword: %s"), *InputKeyword);
			}

			// Remove from the keyword map.
			if (KeywordMap.Contains(InputKeyword))
			{
				KeywordMap.Remove(InputKeyword);
			}
			else
			{
				UE_LOG(LogHMD, Warning, TEXT("FSpeechPlugin::RemoveKeywords failed to remove Keyword from Key Map: %s"), *InputKeyword);
			}
		}

		StartSpeechRecognizer();
	}

	APlayerController* FSpeechPlugin::GetPlayerController()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
			{
				UWorld* World = Context.World();
				if (World && World->GetGameInstance())
				{
					return World->GetGameInstance()->GetFirstLocalPlayerController();
				}
			}
		}

		return nullptr;
	}

	void FSpeechPlugin::RegisterKeyword(FKey Key, FString Keyword)
	{
		TArray<FKey> Keys;
		EKeys::GetAllKeys(Keys);

		// Only add key if it doesn't already exist.
		if (!Keys.Contains(Key))
		{
			EKeys::AddKey(FKeyDetails(Key, FText(), FKeyDetails::NotBlueprintBindableKey, FInputActionSpeechMapping::GetKeyCategory()));
		}

		Keyword = Keyword.ToLower();
		if (KeywordMap.Contains(Keyword))
		{
			UE_LOG(LogHMD, Warning, TEXT("Adding duplicate keyword: %s. Multiple events may be called for this keyword."), *Keyword);
		}

		Keywords.push_back(winrt::hstring(*Keyword));
		KeywordMap.Add(Keyword, Key);
	}

	void FSpeechPlugin::CallSpeechCallback(FKey InKey)
	{
		APlayerController* PlayerController = GetPlayerController();
		if (PlayerController == nullptr)
		{
			UE_LOG(LogHMD, Warning, TEXT("Attempting to call speech keyword, but PlayerController is not valid"));
			return;
		}

		AsyncTask(ENamedThreads::GameThread, [InKey, PlayerController]()
		{ 
			FInputKeyParams key(InKey, IE_Pressed, 1.0, false);
			PlayerController->InputKey(key);
		});
	}

	void FSpeechPlugin::RegisterSpeechCommandsWithRemoting()
	{
		if (!bIsRemotingSpeechExtensionEnabled)
		{
			// Only use the remoting OpenXR extension when remoting.
			return;
		}

#if SUPPORTS_REMOTING
		const TArray <FInputActionSpeechMapping>& SpeechMappings = GetDefault<UInputSettings>()->GetSpeechMappings();

		XrRemotingSpeechInitInfoMSFT remotingSpeechInfo{ (XrStructureType)XR_TYPE_REMOTING_SPEECH_INIT_INFO_MSFT };
		
		// A language string is required for remoting speech to work.
		// Default to "en-US", but allow for additional languages in the game config.
		FString RemotingSpeechLanguage = "en-US";
		GConfig->GetString(TEXT("/Script/MicrosoftOpenXRRemotingSettings"), TEXT("SpeechLanguage"),
			RemotingSpeechLanguage, GGameIni);
		if (RemotingSpeechLanguage.TrimStartAndEnd().IsEmpty())
		{
			// A SpeechLanguage entry exists, but is empty.  Default back to "en-US"
			RemotingSpeechLanguage = "en-US";
		}

		UE_LOG(LogHMD, Log, TEXT("Remoting initializing with language: %s."), *RemotingSpeechLanguage);

		strcpy_s(remotingSpeechInfo.language, TCHAR_TO_UTF8(*RemotingSpeechLanguage));

		std::vector<const char*> keywords;
		for (const FInputActionSpeechMapping& SpeechMapping : SpeechMappings)
		{
			FString key = SpeechMapping.GetSpeechKeyword().ToString();
			int keywordLength = key.Len() + 1;
			char* keyword = new char[keywordLength];
			strcpy_s(keyword, keywordLength * sizeof(char), (const char*)TCHAR_TO_UTF8(*key));

			keywords.push_back(keyword);
		}

		remotingSpeechInfo.dictionaryEntries = keywords.data();
		remotingSpeechInfo.dictionaryEntriesCount = keywords.size();

		XrResult initializationResult = xrInitializeRemotingSpeechMSFT(Session, &remotingSpeechInfo);
		if (XR_FAILED(initializationResult))
		{
			UE_LOG(LogHMD, Warning, TEXT("Remoting speech failed to initialize with XrResult: %d."), initializationResult);
		}

		for (const char* keyword : keywords)
		{
			delete[] keyword;
		}
#endif
	}

	void FSpeechPlugin::StartSpeechRecognizer()
	{
		if (bIsRemotingSpeechExtensionEnabled)
		{
			RegisterSpeechCommandsWithRemoting();
			return;
		}

		SpeechRecognitionListConstraint constraint = SpeechRecognitionListConstraint(Keywords);
		SpeechRecognizer = winrt::Windows::Media::SpeechRecognition::SpeechRecognizer();
		SpeechRecognizer.Constraints().Clear();
		SpeechRecognizer.Constraints().Append(constraint);

		CompileConstraintsAsyncOperation = SpeechRecognizer.CompileConstraintsAsync();

		CompileConstraintsAsyncOperation.Completed([this](winrt::Windows::Foundation::IAsyncOperation<SpeechRecognitionCompilationResult> asyncOperation, winrt::Windows::Foundation::AsyncStatus status)
		{
			if (asyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				SpeechRecognitionCompilationResult result = asyncOperation.GetResults();
				if (result.Status() == SpeechRecognitionResultStatus::Success)
				{
					try
					{
						SpeechContinuousRecognitionSession session = SpeechRecognizer.ContinuousRecognitionSession();
						SessionStartAction = session.StartAsync();
					}
					catch (winrt::hresult_error e)
					{
						// We may see an exception if the microphone capability is not enabled.
						UE_LOG(LogHMD, Warning, TEXT("SpeechRecognizer failed to start with error: %s"), e.message().c_str());
						StopSpeechRecognizer();
					}
				}
				else
				{
					UE_LOG(LogHMD, Warning, TEXT("SpeechRecognizer access request returns error: %d"), result.Status());
					StopSpeechRecognizer();
				}
			}
			else if (asyncOperation.Status() != winrt::Windows::Foundation::AsyncStatus::Canceled)
			{
				UE_LOG(LogHMD, Warning, TEXT("SpeechRecognizer.CompileConstraintsAsync returns error: %d"), asyncOperation.Status());
				StopSpeechRecognizer();
			}
		});

		ResultsGeneratedToken = SpeechRecognizer.ContinuousRecognitionSession().ResultGenerated(
			[&](SpeechContinuousRecognitionSession sender, SpeechContinuousRecognitionResultGeneratedEventArgs args)
		{
			if (args.Result().Status() == SpeechRecognitionResultStatus::Success &&
				args.Result().Confidence() != SpeechRecognitionConfidence::Rejected)
			{
				FString keyword = FString(args.Result().Text().c_str());
				CallSpeechCallback(KeywordMap.FindRef(keyword));
			}
		});
	}

	void FSpeechPlugin::StopSpeechRecognizer()
	{
		if (bIsRemotingSpeechExtensionEnabled)
		{
			// Remoting does not use the winrt SpeechRecognizer
			return;
		}

		if (CompileConstraintsAsyncOperation && CompileConstraintsAsyncOperation.Status() != winrt::Windows::Foundation::AsyncStatus::Completed)
		{
			CompileConstraintsAsyncOperation.Cancel();
		}

		if (SpeechRecognizer != nullptr &&
			ResultsGeneratedToken.value != 0)
		{
			SpeechRecognizer.ContinuousRecognitionSession().ResultGenerated(ResultsGeneratedToken);
			ResultsGeneratedToken.value = 0;
		}

		if (SpeechRecognizer != nullptr &&
			CompileConstraintsAsyncOperation.Status() == winrt::Windows::Foundation::AsyncStatus::Completed)
		{
			// If the SpeechRecognizer is idle, it is not capturing.  Stopping while idle will throw an exception.
			if ((SessionStartAction != nullptr && SessionStartAction.Status() == winrt::Windows::Foundation::AsyncStatus::Completed) &&
				SpeechRecognizer.State() != winrt::Windows::Media::SpeechRecognition::SpeechRecognizerState::Idle)
			{
				try
				{
					SpeechRecognizer.ContinuousRecognitionSession().StopAsync().Completed([this](
						winrt::Windows::Foundation::IAsyncAction action, winrt::Windows::Foundation::AsyncStatus status)
					{
						// close the speech recognizer after the recognition session stops.
						// Otherwise closing the speech recognizer can cause a hang.
						this->SpeechRecognizer.Constraints().Clear();
						this->SpeechRecognizer.Close();
						this->SpeechRecognizer = nullptr;
					});
				}
				catch (winrt::hresult_error e)
				{
					// We may see an exception if no microphone was attached.
					UE_LOG(LogHMD, Warning, TEXT("ContinuousRecognitionSession failed to stop with error: %d"), e.code().value);
				}
			}
			else
			{
				try
				{
					SpeechRecognizer.Constraints().Clear();
					SpeechRecognizer.Close();
					SpeechRecognizer = nullptr;
				}
				catch (winrt::hresult_error e)
				{
					UE_LOG(LogHMD, Warning, TEXT("SpeechRecognizer failed to close with error: %d"), e.code().value);
				}
			}
		}
	}
}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
