// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/UxtTouchBasedHandTrackerSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "HandTracking/UxtTouchBasedHandTrackerComponent.h"

bool UUxtTouchBasedHandTrackerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if PLATFORM_ANDROID
	return true;
#else
	return Outer->GetWorld()->IsPlayInMobilePreview();
#endif
}

void UUxtTouchBasedHandTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtTouchBasedHandTrackerSubsystem::OnGameModePostLogin);
}

void UUxtTouchBasedHandTrackerSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	PostLoginHandle.Reset();
}

void UUxtTouchBasedHandTrackerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	// Add touch-based hand tracker component to player controller
	UUxtTouchBasedHandTrackerComponent* TouchHandTracker = NewObject<UUxtTouchBasedHandTrackerComponent>(NewPlayer);
	TouchHandTracker->RegisterComponent();
}
