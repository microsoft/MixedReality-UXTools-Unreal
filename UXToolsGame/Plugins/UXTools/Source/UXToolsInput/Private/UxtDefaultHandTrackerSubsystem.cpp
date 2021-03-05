// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtDefaultHandTrackerSubsystem.h"

#include "ARSupportInterface.h"
#include "IXRTrackingSystem.h"
#include "SceneViewExtension.h"
#include "UxtXRSimulationViewExtension.h"
#include "XRSimulationActor.h"
#include "XRSimulationRuntimeSettings.h"
#include "XRSimulationState.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Features/IModularFeatures.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

void UUxtDefaultHandTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FUxtDefaultHandTracker::RegisterInputMappings();

	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModeLogout);

	AXRSimulationActor::RegisterInputMappings();
	SimulationState = MakeShareable(new FXRSimulationState());

	XRSimulationViewExtension = FSceneViewExtensions::NewExtension<FUxtXRSimulationViewExtension>(this);
}

void UUxtDefaultHandTrackerSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	PostLoginHandle.Reset();
	LogoutHandle.Reset();

	FUxtDefaultHandTracker::UnregisterInputMappings();

	AXRSimulationActor::UnregisterInputMappings();
	SimulationState.Reset();

	XRSimulationViewExtension = nullptr;
}

void UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->IsLocalController())
	{
		if (NewPlayer->InputComponent)
		{
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::LeftSelect, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnLeftSelectPressed);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::LeftSelect, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnLeftSelectReleased);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::LeftGrab, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnLeftGripPressed);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::LeftGrab, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnLeftGripReleased);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::RightSelect, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnRightSelectPressed);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::RightSelect, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnRightSelectReleased);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::RightGrab, IE_Pressed, this, &UUxtDefaultHandTrackerSubsystem::OnRightGripPressed);
			NewPlayer->InputComponent->BindAction(
				UxtHandTrackerInputActions::RightGrab, IE_Released, this, &UUxtDefaultHandTrackerSubsystem::OnRightGripReleased);
		}

		// Tick handler for updating the cached motion controller data.
		// Using OnWorldPreActorTick here which runs after OnWorldTickStart, at which point XR systems should have updated all controller
		// data.
		TickDelegateHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnWorldPreActorTick);

		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &DefaultHandTracker);

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

void UUxtDefaultHandTrackerSubsystem::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		if (PlayerController->IsLocalController())
		{
			IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &DefaultHandTracker);

			FWorldDelegates::OnWorldPreActorTick.Remove(TickDelegateHandle);

			if (PlayerController->InputComponent)
			{
				PlayerController->InputComponent->AxisBindings.RemoveAll(
					[this](const FInputAxisBinding& Binding) -> bool { return Binding.AxisDelegate.IsBoundToObject(this); });
			}

			DestroyInputSimActor();
			DestroyHmdCameraActor();
			bSimulationEnabled = false;
		}
	}
}

void UUxtDefaultHandTrackerSubsystem::OnWorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	if (bSimulationEnabled)
	{
		// Use simulated data generated by the simulation actor
		AXRSimulationActor* SimActor = SimulationActorWeak.Get();
		if (ensure(SimActor))
		{
			SimActor->GetHandData(EControllerHand::Left, DefaultHandTracker.ControllerData_Left);
			SimActor->GetHandData(EControllerHand::Right, DefaultHandTracker.ControllerData_Right);

			// Update Select/Grip state directly, no input events are used here
			SimActor->GetControllerActionState(
				EControllerHand::Left, DefaultHandTracker.bIsSelectPressed_Left, DefaultHandTracker.bIsGrabbing_Left);
			SimActor->GetControllerActionState(
				EControllerHand::Right, DefaultHandTracker.bIsSelectPressed_Right, DefaultHandTracker.bIsGrabbing_Right);

			// Head pose is using the XRTrackingSystem as well, force override in the function library
			UUxtFunctionLibrary::bUseTestData = true;
			FVector HeadPosition;
			FQuat HeadRotation;
			SimActor->GetHeadPose(HeadRotation, HeadPosition);
			UUxtFunctionLibrary::TestHeadPose = FTransform(HeadRotation, HeadPosition);
		}
	}
	else
	{
		// True XR system data from devices
		if (IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get())
		{
			XRSystem->GetMotionControllerData(World, EControllerHand::Left, DefaultHandTracker.ControllerData_Left);
			XRSystem->GetMotionControllerData(World, EControllerHand::Right, DefaultHandTracker.ControllerData_Right);
		}

		// Disable head pose override from simulation
		UUxtFunctionLibrary::bUseTestData = false;
	}
}

void UUxtDefaultHandTrackerSubsystem::OnLeftSelectPressed()
{
	DefaultHandTracker.bIsSelectPressed_Left = true;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftSelectReleased()
{
	DefaultHandTracker.bIsSelectPressed_Left = false;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftGripPressed()
{
	DefaultHandTracker.bIsGrabbing_Left = true;
}

void UUxtDefaultHandTrackerSubsystem::OnLeftGripReleased()
{
	DefaultHandTracker.bIsGrabbing_Left = false;
}

void UUxtDefaultHandTrackerSubsystem::OnRightSelectPressed()
{
	DefaultHandTracker.bIsSelectPressed_Right = true;
}

void UUxtDefaultHandTrackerSubsystem::OnRightSelectReleased()
{
	DefaultHandTracker.bIsSelectPressed_Right = false;
}

void UUxtDefaultHandTrackerSubsystem::OnRightGripPressed()
{
	DefaultHandTracker.bIsGrabbing_Right = true;
}

void UUxtDefaultHandTrackerSubsystem::OnRightGripReleased()
{
	DefaultHandTracker.bIsGrabbing_Right = false;
}

AXRSimulationActor* UUxtDefaultHandTrackerSubsystem::GetOrCreateInputSimActor(APlayerController* PlayerController)
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

void UUxtDefaultHandTrackerSubsystem::DestroyInputSimActor()
{
	if (AActor* SimulationActor = SimulationActorWeak.Get())
	{
		SimulationActor->Destroy();
	}
	SimulationActorWeak.Reset();
}

AActor* UUxtDefaultHandTrackerSubsystem::GetOrCreateHmdCameraActor(APlayerController* PlayerController)
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

void UUxtDefaultHandTrackerSubsystem::DestroyHmdCameraActor()
{
	if (AActor* HmdCameraActor = HmdCameraActorWeak.Get())
	{
		HmdCameraActor->Destroy();
	}
	HmdCameraActorWeak.Reset();
}
