// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationGameInstanceSubsystem.h"
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

bool UUxtInputSimulationGameInstanceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return UWindowsMixedRealityInputSimulationEngineSubsystem::IsInputSimulationEnabled();
}

void UUxtInputSimulationGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	CreateInputSimActor();
	CreateHmdCameraActor();
}

void UUxtInputSimulationGameInstanceSubsystem::Deinitialize()
{
	DestroyInputSimActor();
	DestroyHmdCameraActor();
}

void UUxtInputSimulationGameInstanceSubsystem::CreateInputSimActor()
{
	UWorld* World = GetWorld();
	check(World);

	if (!InputSimActor)
	{
		FActorSpawnParameters p;
		p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		InputSimActor = World->SpawnActor<AUxtInputSimulationActor>(p);
#if WITH_EDITOR
		InputSimActor->SetActorLabel(TEXT("InputSimulation"));
#endif
	}
}

void UUxtInputSimulationGameInstanceSubsystem::DestroyInputSimActor()
{
	if (InputSimActor)
	{
		InputSimActor->Destroy();
		InputSimActor = nullptr;
	}
}

void UUxtInputSimulationGameInstanceSubsystem::CreateHmdCameraActor()
{
	UWorld* World = GetWorld();
	check(World);

	if (!HmdCameraActor)
	{
		FActorSpawnParameters p;
		p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		HmdCameraActor = World->SpawnActor<AActor>(p);
#if WITH_EDITOR
		HmdCameraActor->SetActorLabel(TEXT("HmdCamera"));
#endif

		HmdCameraComponent = NewObject<UCameraComponent>(HmdCameraActor, TEXT("Camera"));
		HmdCameraActor->AddOwnedComponent(HmdCameraComponent);
		HmdCameraComponent->SetupAttachment(HmdCameraActor->GetRootComponent());
		HmdCameraComponent->RegisterComponent();
		// Camera should use HMD location
		HmdCameraComponent->bLockToHmd = true;

		// Subscribe to the PostLogin event to set the view target after the local player controller is created.
		FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtInputSimulationGameInstanceSubsystem::OnGameModePostLogin);
	}
}

void UUxtInputSimulationGameInstanceSubsystem::DestroyHmdCameraActor()
{
	if (HmdCameraActor)
	{
		HmdCameraActor->Destroy();
		HmdCameraActor = nullptr;
	}
}

void UUxtInputSimulationGameInstanceSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->Player == GetLocalPlayer())
	{
		// Set the view target for the player controller to render from the HMD position
		NewPlayer->SetViewTarget(HmdCameraActor);
		// Prevent the controller from resetting the view target when the player is restarted
		NewPlayer->bAutoManageActiveCameraTarget = false;
	}
}

#undef LOCTEXT_NAMESPACE
