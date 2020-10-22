#include "SpeechPlugin.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

using namespace winrt::Windows::Media::SpeechRecognition;

#pragma warning(push)
#pragma warning(disable : 4265)
#pragma warning(disable : 4946)

namespace MRPlatExt
{
	void FSpeechPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FSpeechPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	void FSpeechPlugin::OnStartARSession(class UARSessionConfig* SessionConfig)
	{
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
	}

	void FSpeechPlugin::OnStopARSession()
	{
		//remove keys from "speech" namespace
		EKeys::RemoveKeysWithCategory(FInputActionSpeechMapping::GetKeyCategory());

		StopSpeechRecognizer();

		KeywordMap.Empty();
	}

	void FSpeechPlugin::AddKeywords(TArray<FKeywordInput> KeywordsToAdd)
	{
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
			PlayerController->InputKey(InKey, IE_Pressed, 1.0f, false);
		});
	}

	void FSpeechPlugin::StartSpeechRecognizer()
	{
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
			if (SessionStartAction.Status() == winrt::Windows::Foundation::AsyncStatus::Completed &&
				SpeechRecognizer.State() != winrt::Windows::Media::SpeechRecognition::SpeechRecognizerState::Idle)
			{
				try
				{
					SpeechRecognizer.ContinuousRecognitionSession().StopAsync();
				}
				catch (winrt::hresult_error e)
				{
					// We may see an exception if no microphone was attached.
					UE_LOG(LogHMD, Warning, TEXT("ContinuousRecognitionSession failed to stop with error: %d"), e.code().value);
				}
			}

			SpeechRecognizer.Constraints().Clear();
			SpeechRecognizer.Close();
			SpeechRecognizer = nullptr;
		}
	}
}	 // namespace MRPlatExt

#pragma warning(pop)

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
