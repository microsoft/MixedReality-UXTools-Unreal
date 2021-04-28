// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtDefaultHandTrackerSubsystem.h"

#include "ARSupportInterface.h"
#include "IXRTrackingSystem.h"
#include "SceneViewExtension.h"
#include "UxtXRSimulationSubsystem.h"
#include "XRSimulationActor.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Features/IModularFeatures.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/UxtFunctionLibrary.h"

void UUxtDefaultHandTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FUxtDefaultHandTracker::RegisterInputMappings();
	AXRSimulationActor::RegisterInputMappings();

	PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModePostLogin);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnGameModeLogout);
}

void UUxtDefaultHandTrackerSubsystem::Deinitialize()
{
	FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	PostLoginHandle.Reset();
	LogoutHandle.Reset();

	FUxtDefaultHandTracker::UnregisterInputMappings();
	AXRSimulationActor::UnregisterInputMappings();
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

		if (IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get())
		{
			// Check if the project is using the WMR plugin and currently running on an Unreal version earlier than either 4.26.3 or 4.27.
			// WMR in earlier versions has a known incompatibility with WindowsMixedReality grip poses not applying the world scale.
			const FEngineVersion EngineVersion = FEngineVersion::Current();
			const FEngineVersionBase GripPoseFixVersion = FEngineVersionBase(4, 26, 3);
			EVersionComponent VersionComponent;
			ShouldApplyGripPoseScale =
				FEngineVersion::GetNewest(EngineVersion, GripPoseFixVersion, &VersionComponent) == EVersionComparison::Second &&
				XRSystem->GetSystemName().ToString().ToLower().TrimStartAndEnd() == "windowsmixedrealityhmd";
		}

		// Tick handler for updating the cached motion controller data.
		// Using OnWorldPreActorTick here which runs after OnWorldTickStart, at which point XR systems should have updated all controller
		// data.
		TickDelegateHandle = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UUxtDefaultHandTrackerSubsystem::OnWorldPreActorTick);

		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &DefaultHandTracker);
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
		}
	}
}

void UUxtDefaultHandTrackerSubsystem::OnWorldPreActorTick(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	UUxtXRSimulationSubsystem* XRSimulationSubsystem = nullptr;
	if (APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			XRSimulationSubsystem = LocalPlayer->GetSubsystem<UUxtXRSimulationSubsystem>();
		}
	}

	if (XRSimulationSubsystem && XRSimulationSubsystem->IsSimulationEnabled())
	{
		// Use simulated data generated by the simulation actor
		// Update Select/Grip state directly, no input events are used here
		XRSimulationSubsystem->GetMotionControllerData(
			EControllerHand::Left, DefaultHandTracker.ControllerData_Left, DefaultHandTracker.bIsSelectPressed_Left,
			DefaultHandTracker.bIsGrabbing_Left);
		XRSimulationSubsystem->GetMotionControllerData(
			EControllerHand::Right, DefaultHandTracker.ControllerData_Right, DefaultHandTracker.bIsSelectPressed_Right,
			DefaultHandTracker.bIsGrabbing_Right);

		// Head pose is using the XRTrackingSystem as well, force override in the function library
		FVector HeadPosition;
		FQuat HeadRotation;
		XRSimulationSubsystem->GetHeadPose(HeadRotation, HeadPosition);
		UUxtFunctionLibrary::bUseTestData = true;
		UUxtFunctionLibrary::TestHeadPose = FTransform(HeadRotation, HeadPosition);
	}
	else
	{
		// True XR system data from devices
		if (IXRTrackingSystem* XRSystem = GEngine->XRSystem.Get())
		{
			XRSystem->GetMotionControllerData(World, EControllerHand::Left, DefaultHandTracker.ControllerData_Left);
			XRSystem->GetMotionControllerData(World, EControllerHand::Right, DefaultHandTracker.ControllerData_Right);

			// Work around a known bug with WindowsMixedReality not scaling the grip pose from GetMotionControllerData.
			if (ShouldApplyGripPoseScale)
			{
				float Scale = XRSystem->GetWorldToMetersScale();
				FTransform TrackingToWorld = XRSystem->GetTrackingToWorldTransform();
				FTransform LeftGripTransform =
					FTransform(
						DefaultHandTracker.ControllerData_Left.GripRotation, DefaultHandTracker.ControllerData_Left.GripPosition * Scale) *
					TrackingToWorld;
				FTransform RightGripTransform = FTransform(
													DefaultHandTracker.ControllerData_Right.GripRotation,
													DefaultHandTracker.ControllerData_Right.GripPosition * Scale) *
												TrackingToWorld;

				DefaultHandTracker.ControllerData_Left.GripRotation = LeftGripTransform.GetRotation();
				DefaultHandTracker.ControllerData_Left.GripPosition = LeftGripTransform.GetLocation();

				DefaultHandTracker.ControllerData_Right.GripRotation = RightGripTransform.GetRotation();
				DefaultHandTracker.ControllerData_Right.GripPosition = RightGripTransform.GetLocation();
			}
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
