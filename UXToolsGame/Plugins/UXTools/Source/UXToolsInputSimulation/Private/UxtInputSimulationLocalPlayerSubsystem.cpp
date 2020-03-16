// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationLocalPlayerSubsystem.h"
#include "UxtInputSimulationActor.h"

#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"

#include "IHeadMountedDisplay.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "UXToolsInputSimulation"

bool UUxtInputSimulationLocalPlayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return UWindowsMixedRealityInputSimulationEngineSubsystem::IsInputSimulationEnabled();
}

void UUxtInputSimulationLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// Subscribe to PostLoadMap event to recreate the actors after a map has been destroyed.
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UUxtInputSimulationLocalPlayerSubsystem::OnPostLoadMapWithWorld);

	// Subscribe to the PostLogin event to set the view target after the local player controller is created.
	FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtInputSimulationLocalPlayerSubsystem::OnGameModePostLogin);

	if (UWorld* World = GetWorld())
	{
		CreateActors(World);
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::Deinitialize()
{
	DestroyInputSimActor();
	DestroyHmdCameraActor();
}

void UUxtInputSimulationLocalPlayerSubsystem::CreateActors(UWorld* World)
{
	if (World && World->IsPlayInEditor())
	{
		CreateInputSimActor(World);
		CreateHmdCameraActor(World);

		ULocalPlayer* Player = GetLocalPlayer();
		if (Player)
		{
			APlayerController* PC = Player->GetPlayerController(World);
			if (PC)
			{
				SetPlayerCameraTarget(PC);
			}
		}
	}
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
		HmdCameraComponent->SetupAttachment(HmdCameraActor->GetRootComponent());
		HmdCameraComponent->RegisterComponent();
		// Camera should use HMD location
		HmdCameraComponent->bLockToHmd = true;

		HmdCameraActorWeak = HmdCameraActor;
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

void UUxtInputSimulationLocalPlayerSubsystem::SetPlayerCameraTarget(APlayerController* PlayerController)
{
	check(PlayerController);
	if (AActor* HmdCameraActor = HmdCameraActorWeak.Get())
	{
		// Set the view target for the player controller to render from the HMD position
		PlayerController->SetViewTarget(HmdCameraActor);
		// Prevent the controller from resetting the view target when the player is restarted
		PlayerController->bAutoManageActiveCameraTarget = false;
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->Player == GetLocalPlayer())
	{
		SetPlayerCameraTarget(NewPlayer);
	}
}

void UUxtInputSimulationLocalPlayerSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	CreateActors(LoadedWorld);
}


#undef LOCTEXT_NAMESPACE
