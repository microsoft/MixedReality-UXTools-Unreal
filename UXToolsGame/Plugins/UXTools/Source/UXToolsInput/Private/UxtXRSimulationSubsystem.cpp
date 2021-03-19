// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtXRSimulationSubsystem.h"

#include "SceneViewExtension.h"
#include "UxtXRSimulationViewExtension.h"
#include "XRSimulationActor.h"
#include "XRSimulationRuntimeSettings.h"

#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

void UUxtXRSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtXRSimulationSubsystem::OnGameModePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtXRSimulationSubsystem::OnGameModeLogout);

	SimulationState = MakeShareable(new FXRSimulationState());

	XRSimulationViewExtension = FSceneViewExtensions::NewExtension<FUxtXRSimulationViewExtension>(this);
}

void UUxtXRSimulationSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	PostLoginHandle.Reset();
	LogoutHandle.Reset();

	SimulationState.Reset();

	XRSimulationViewExtension = nullptr;
}

bool UUxtXRSimulationSubsystem::GetMotionControllerData(
	EControllerHand Hand, FXRMotionControllerData& OutMotionControllerData, bool& OutSelectPressed, bool& OutGrabbing) const
{
	AXRSimulationActor* SimActor = SimulationActorWeak.Get();
	if (ensure(SimActor))
	{
		SimActor->GetHandData(Hand, OutMotionControllerData);
		SimActor->GetControllerActionState(Hand, OutSelectPressed, OutGrabbing);
		return true;
	}
	return false;
}

bool UUxtXRSimulationSubsystem::GetHeadPose(FQuat& OutHeadRotation, FVector& OutHeadPosition) const
{
	AXRSimulationActor* SimActor = SimulationActorWeak.Get();
	if (ensure(SimActor))
	{
		SimActor->GetHeadPose(OutHeadRotation, OutHeadPosition);
		return true;
	}
	return false;
}

void UUxtXRSimulationSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->IsLocalController())
	{
		bSimulationEnabled = false;
		const UXRSimulationRuntimeSettings* Settings = UXRSimulationRuntimeSettings::Get();
		if (Settings->bEnableSimulation)
		{
			// Don't use input simulation in VR preview mode and only in PIE worlds.
			bool bIsPreview = false;
#if WITH_EDITOR
			if (GIsEditor)
			{
				UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
				if (EdEngine->GetPlayInEditorSessionInfo().IsSet())
				{
					EPlaySessionPreviewType SessionPreviewType =
						EdEngine->GetPlayInEditorSessionInfo()->OriginalRequestParams.SessionPreviewTypeOverride.Get(
							EPlaySessionPreviewType::NoPreview);
					bIsPreview = SessionPreviewType != EPlaySessionPreviewType::NoPreview;
				}
			}
#endif
			bSimulationEnabled = NewPlayer->GetWorld()->WorldType == EWorldType::PIE && !bIsPreview;
		}

		if (bSimulationEnabled)
		{
			GetOrCreateInputSimActor(NewPlayer);
			GetOrCreateHmdCameraActor(NewPlayer);
		}
	}
}

void UUxtXRSimulationSubsystem::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (PlayerController->IsLocalController())
		{
			DestroyInputSimActor();
			DestroyHmdCameraActor();
			bSimulationEnabled = false;
		}
	}
}

AXRSimulationActor* UUxtXRSimulationSubsystem::GetOrCreateInputSimActor(APlayerController* PlayerController)
{
	// Only create one actor
	if (SimulationActorWeak.IsValid())
	{
		return SimulationActorWeak.Get();
	}

	FActorSpawnParameters p;
	p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	p.bDeferConstruction = true;

	UWorld* World = PlayerController->GetWorld();
	AXRSimulationActor* SimulationActor = World->SpawnActor<AXRSimulationActor>(p);
	SimulationActorWeak = SimulationActor;

	SimulationActor->SetSimulationState(SimulationState);

	// Explicitly enable input: The simulation actor may be created after loading a map,
	// in which case auto-enabling input does not work.
	SimulationActor->EnableInput(PlayerController);

	// Attach to the player controller so the start location matches the HMD camera, which is relative to the controller.
	SimulationActor->AttachToActor(PlayerController, FAttachmentTransformRules::KeepRelativeTransform);

	UGameplayStatics::FinishSpawningActor(SimulationActor, FTransform::Identity);

	return SimulationActor;
}

void UUxtXRSimulationSubsystem::DestroyInputSimActor()
{
	if (AActor* SimulationActor = SimulationActorWeak.Get())
	{
		SimulationActor->Destroy();
	}
	SimulationActorWeak.Reset();
}

AActor* UUxtXRSimulationSubsystem::GetOrCreateHmdCameraActor(APlayerController* PlayerController)
{
	if (HmdCameraActorWeak.IsValid())
	{
		return HmdCameraActorWeak.Get();
	}

	FActorSpawnParameters p;
	p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UWorld* World = PlayerController->GetWorld();
	AActor* HmdCameraActor = World->SpawnActor<AActor>(p);
	HmdCameraActorWeak = HmdCameraActor;
#if WITH_EDITOR
	HmdCameraActor->SetActorLabel(TEXT("HmdCamera"));
#endif

	UCameraComponent* HmdCameraComponent = NewObject<UCameraComponent>(HmdCameraActor, TEXT("HmdCamera"));
	HmdCameraActor->AddOwnedComponent(HmdCameraComponent);
	HmdCameraActor->SetRootComponent(HmdCameraComponent);
	HmdCameraComponent->RegisterComponent();
	// Camera should use HMD location
	// XXX LockToHmd worked in the previous approach where the actor defined the HMD positions.
	// XXX When using this without a HMD we need to attach to the sim actor instead.
	/*HmdCameraComponent->bLockToHmd = true;*/
	if (ensure(SimulationActorWeak.IsValid()))
	{
		HmdCameraActor->AttachToActor(SimulationActorWeak.Get(), FAttachmentTransformRules::KeepRelativeTransform);
	}

	// Set the view target for the player controller to render from the HMD position
	PlayerController->SetViewTarget(HmdCameraActor);
	// Prevent the controller from resetting the view target when the player is restarted
	PlayerController->bAutoManageActiveCameraTarget = false;

	return HmdCameraActor;
}

void UUxtXRSimulationSubsystem::DestroyHmdCameraActor()
{
	if (AActor* HmdCameraActor = HmdCameraActorWeak.Get())
	{
		HmdCameraActor->Destroy();
	}
	HmdCameraActorWeak.Reset();
}
