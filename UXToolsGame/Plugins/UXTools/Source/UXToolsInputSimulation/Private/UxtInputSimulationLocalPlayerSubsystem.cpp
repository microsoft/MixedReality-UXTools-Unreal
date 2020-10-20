// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationLocalPlayerSubsystem.h"

#include "IHeadMountedDisplay.h"
#include "UxtInputSimulationActor.h"
#include "UxtInputSimulationState.h"
#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "UXToolsInputSimulation"

UUxtInputSimulationState* UUxtInputSimulationLocalPlayerSubsystem::GetSimulationState() const
{
	return SimulationState;
}

bool UUxtInputSimulationLocalPlayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return UWindowsMixedRealityInputSimulationEngineSubsystem::IsInputSimulationEnabled();
}

void UUxtInputSimulationLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	SimulationState = NewObject<UUxtInputSimulationState>();

	// Subscribe to the PostLogin event to set the view target after the local player controller is created.
	FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtInputSimulationLocalPlayerSubsystem::OnGameModePostLogin);
	// Destroy actors after logout
	FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtInputSimulationLocalPlayerSubsystem::OnGameModeLogout);

	// Subscribe to PostLoadMap event to recreate the actors after a map has been destroyed.
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UUxtInputSimulationLocalPlayerSubsystem::OnPostLoadMapWithWorld);
}

void UUxtInputSimulationLocalPlayerSubsystem::Deinitialize()
{
	SimulationState = nullptr;
}

void UUxtInputSimulationLocalPlayerSubsystem::CreateInputSimActor(UWorld* World)
{
	if (!InputSimActorWeak.IsValid())
	{
		FActorSpawnParameters p;
		p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* InputSimActor = World->SpawnActor<AUxtInputSimulationActor>(p);

		InputSimActorWeak = InputSimActor;
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::CreateHmdCameraActor(UWorld* World)
{
	if (!HmdCameraActorWeak.IsValid())
	{
		FActorSpawnParameters p;
		p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* HmdCameraActor = World->SpawnActor<AActor>(p);
#if WITH_EDITOR
		HmdCameraActor->SetActorLabel(TEXT("HmdCamera"));
#endif

		UCameraComponent* HmdCameraComponent = NewObject<UCameraComponent>(HmdCameraActor, TEXT("Camera"));
		HmdCameraActor->AddOwnedComponent(HmdCameraComponent);
		HmdCameraActor->SetRootComponent(HmdCameraComponent);
		HmdCameraComponent->RegisterComponent();
		// Camera should use HMD location
		HmdCameraComponent->bLockToHmd = true;

		HmdCameraActorWeak = HmdCameraActor;
	}

	// Set the HmdCameraActor as the view target for the player controller
	if (AActor* HmdCameraActor = HmdCameraActorWeak.Get())
	{
		ULocalPlayer* Player = GetLocalPlayer();
		if (Player)
		{
			APlayerController* PlayerController = Player->GetPlayerController(World);
			if (PlayerController)
			{
				// Set the view target for the player controller to render from the HMD position
				PlayerController->SetViewTarget(HmdCameraActor);
				// Prevent the controller from resetting the view target when the player is restarted
				PlayerController->bAutoManageActiveCameraTarget = false;
			}
		}
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::DestroyInputSimActor()
{
	if (AActor* InputSimActor = InputSimActorWeak.Get())
	{
		InputSimActor->Destroy();
	}
	InputSimActorWeak.Reset();
}

void UUxtInputSimulationLocalPlayerSubsystem::DestroyHmdCameraActor()
{
	if (AActor* HmdCameraActor = HmdCameraActorWeak.Get())
	{
		HmdCameraActor->Destroy();
	}
	HmdCameraActorWeak.Reset();
}

void UUxtInputSimulationLocalPlayerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->Player == GetLocalPlayer())
	{
		CreateInputSimActor(GetWorld());
		CreateHmdCameraActor(GetWorld());
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (PlayerController->Player == GetLocalPlayer())
		{
			DestroyInputSimActor();
			DestroyHmdCameraActor();
		}
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	CreateInputSimActor(LoadedWorld);
	CreateHmdCameraActor(LoadedWorld);
}

#undef LOCTEXT_NAMESPACE
