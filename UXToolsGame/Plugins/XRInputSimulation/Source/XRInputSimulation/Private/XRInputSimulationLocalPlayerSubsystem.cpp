// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRInputSimulationLocalPlayerSubsystem.h"

#include "IHeadMountedDisplay.h"
#include "XRInputSimulationActor.h"
#include "XRInputSimulationState.h"
#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "XRInputSimulation"

UXRInputSimulationState* UXRInputSimulationLocalPlayerSubsystem::GetSimulationState() const
{
	return SimulationState;
}

bool UXRInputSimulationLocalPlayerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return UWindowsMixedRealityInputSimulationEngineSubsystem::IsInputSimulationEnabled();
}

void UXRInputSimulationLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	SimulationState = NewObject<UXRInputSimulationState>();

	// Subscribe to the PostLogin event to set the view target after the local player controller is created.
	FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UXRInputSimulationLocalPlayerSubsystem::OnGameModePostLogin);
	// Destroy actors after logout
	FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UXRInputSimulationLocalPlayerSubsystem::OnGameModeLogout);

	// Subscribe to PostLoadMap event to recreate the actors after a map has been destroyed.
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UXRInputSimulationLocalPlayerSubsystem::OnPostLoadMapWithWorld);
}

void UXRInputSimulationLocalPlayerSubsystem::Deinitialize()
{
	SimulationState = nullptr;
}

void UXRInputSimulationLocalPlayerSubsystem::CreateInputSimActor(UWorld* World)
{
	if (!InputSimActorWeak.IsValid())
	{
		FActorSpawnParameters p;
		p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* InputSimActor = World->SpawnActor<AXRInputSimulationActor>(p);

		InputSimActorWeak = InputSimActor;
	}
}

void UXRInputSimulationLocalPlayerSubsystem::CreateHmdCameraActor(UWorld* World)
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

void UXRInputSimulationLocalPlayerSubsystem::DestroyInputSimActor()
{
	if (AActor* InputSimActor = InputSimActorWeak.Get())
	{
		InputSimActor->Destroy();
	}
	InputSimActorWeak.Reset();
}

void UXRInputSimulationLocalPlayerSubsystem::DestroyHmdCameraActor()
{
	if (AActor* HmdCameraActor = HmdCameraActorWeak.Get())
	{
		HmdCameraActor->Destroy();
	}
	HmdCameraActorWeak.Reset();
}

void UXRInputSimulationLocalPlayerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->Player == GetLocalPlayer())
	{
		CreateInputSimActor(GetWorld());
		CreateHmdCameraActor(GetWorld());
	}
}

void UXRInputSimulationLocalPlayerSubsystem::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
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

void UXRInputSimulationLocalPlayerSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	CreateInputSimActor(LoadedWorld);
	CreateHmdCameraActor(LoadedWorld);
}

#undef LOCTEXT_NAMESPACE
