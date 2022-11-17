// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRSimulationActor.h"

#include "HeadMountedDisplayTypes.h"
#include "XRSimulationHeadMovementComponent.h"
#include "XRSimulationRuntimeSettings.h"

#include "Animation/AnimBlueprint.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/RuntimeErrors.h"

#define LOCTEXT_NAMESPACE "XRSimulation"

DEFINE_LOG_CATEGORY_STATIC(LogXRSimulationActor, Log, All);

namespace
{
	/** Utility for building a static list of all keypoint enum values. */
	TArray<EHandKeypoint> BuildHandKeypointList()
	{
		const UEnum* KeypointEnum = StaticEnum<EHandKeypoint>();

		TArray<EHandKeypoint> Result;
		Result.SetNumUninitialized(EHandKeypointCount);
		for (int32 iKeypoint = 0; iKeypoint < EHandKeypointCount; ++iKeypoint)
		{
			Result[iKeypoint] = (EHandKeypoint)KeypointEnum->GetValueByIndex(iKeypoint);
		}

		return Result;
	}
} // namespace

AXRSimulationActor::AXRSimulationActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Tick after updates to copy data to the simulated HMD
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;

	// Handles local player input
	AutoReceiveInput = EAutoReceiveInput::Player0;
	bAddDefaultInputBindings = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	AddOwnedComponent(Root);
	SetRootComponent(Root);
	Root->SetMobility(EComponentMobility::Movable);

	SetupHeadComponents();
	SetupHandComponents();

	InputMappingContext = NewObject<UInputMappingContext>();
	InputMappingContext->AddToRoot();

	if (GWorld)
	{
		if (const ULocalPlayer* FirstLocalPlayer = GWorld->GetFirstLocalPlayerFromController())
		{
			UEnhancedInputLocalPlayerSubsystem* EnhancedInputSystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(FirstLocalPlayer);
			if (EnhancedInputSystem && !EnhancedInputSystem->HasMappingContext(InputMappingContext))
			{
				EnhancedInputSystem->AddMappingContext(InputMappingContext, 0);
				RegisterInputMappings();
			}
		}
	}
}

void AXRSimulationActor::SetSimulationState(const TSharedPtr<FXRSimulationState>& NewSimulationState)
{
	SimulationState = NewSimulationState;
}

void AXRSimulationActor::SetTrackingToWorldTransform(const FTransform& InTrackingToWorldTransform)
{
	TrackingToWorldTransform = InTrackingToWorldTransform;
}

void AXRSimulationActor::GetTargetHandPose(
	UObject* WorldContextObject, EControllerHand Hand, FName& TargetPose, FTransform& TargetTransform, bool& bAnimateTransform) const
{
	if (SimulationState.IsValid())
	{
		TargetPose = SimulationState->GetTargetPose(Hand);
		SimulationState->GetTargetHandTransform(Hand, TargetTransform, bAnimateTransform);
	}
	else
	{
		TargetPose = NAME_None;
		TargetTransform = FTransform::Identity;
		bAnimateTransform = false;
	}

	// Sim state transform is relative to the HMD, tracking-to-world transform must be added for full transform.
	const FTransform ParentTransform = GetActorTransform();
	const FTransform WorldTargetTransform = TargetTransform * ParentTransform * TrackingToWorldTransform;
	TargetTransform = WorldTargetTransform.GetRelativeTransform(ParentTransform);
}

void AXRSimulationActor::GetHeadPose(FQuat& Orientation, FVector& Position) const
{
	Orientation = GetActorQuat();
	Position = GetActorLocation();
}

void AXRSimulationActor::GetHandData(EControllerHand Hand, FXRMotionControllerData& MotionControllerData) const
{
	const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();
	check(Settings);

	USkeletalMeshComponent* MeshComp = GetHandMesh(Hand);
	const UEnum* KeypointEnum = StaticEnum<EHandKeypoint>();

	if (!SimulationState.IsValid() || !ensureAsRuntimeWarning(MeshComp != nullptr))
	{
		MotionControllerData.bValid = false;
		return;
	}

	// Visible hands are considered tracked
	const bool bIsTracked = SimulationState->IsHandVisible(Hand);
	MotionControllerData.TrackingStatus = bIsTracked ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;

	MotionControllerData.DeviceVisualType = EXRVisualType::Hand;

	// Transforms needed for aim/grip pose
	FTransform WristTransform = MeshComp->GetComponentTransform();
	FTransform IndexKnuckleTransform = MeshComp->GetComponentTransform();
	FTransform PalmTransform = MeshComp->GetComponentTransform();

	if (!bIsTracked)
	{
		// When untracked the keypoint arrays should be empty
		MotionControllerData.HandKeyPositions.Empty();
		MotionControllerData.HandKeyRotations.Empty();
		MotionControllerData.HandKeyRadii.Empty();
	}
	else
	{
		MotionControllerData.HandKeyPositions.SetNum(EHandKeypointCount);
		MotionControllerData.HandKeyRotations.SetNum(EHandKeypointCount);
		MotionControllerData.HandKeyRadii.SetNum(EHandKeypointCount);

		// Get keypoint transforms for all hand joints
		static const TArray<EHandKeypoint> AllKeypoints = BuildHandKeypointList();
		TArray<FTransform> AllKeypointTransforms;
		TArray<float> AllKeypointRadii;
		GetKeypointTransforms(Hand, AllKeypoints, AllKeypointTransforms, AllKeypointRadii);

		for (int32 i = 0; i < EHandKeypointCount; ++i)
		{
			const EHandKeypoint Keypoint = (EHandKeypoint)KeypointEnum->GetValueByIndex(i);
			check(AllKeypoints[i] == Keypoint);
			const FTransform& KeypointTransform = AllKeypointTransforms[i];
			const float KeypointRadius = AllKeypointRadii[i];

			MotionControllerData.HandKeyPositions[i] = KeypointTransform.GetLocation();
			MotionControllerData.HandKeyRotations[i] = KeypointTransform.GetRotation();
			MotionControllerData.HandKeyRadii[i] = KeypointRadius;
		}

		// Keep special transforms for aim/grip computation below
		WristTransform = AllKeypointTransforms[(int32)EHandKeypoint::Wrist];
		IndexKnuckleTransform = AllKeypointTransforms[(int32)EHandKeypoint::IndexProximal];
		PalmTransform = AllKeypointTransforms[(int32)EHandKeypoint::Palm];
	}

	// Build aim pose from bones
	{
		FVector ShoulderPos =
			(Hand == EControllerHand::Left ? Settings->ShoulderPosition.MirrorByVector(FVector::RightVector) : Settings->ShoulderPosition);
		if (const USceneComponent* ParentComp = MeshComp->GetAttachParent())
		{
			ShoulderPos = ParentComp->GetComponentTransform().TransformPosition(ShoulderPos);
		}

		MotionControllerData.AimPosition = IndexKnuckleTransform.GetLocation();

		const FVector AimDirection = (MeshComp->GetComponentLocation() - ShoulderPos).GetSafeNormal();
		const FVector AimRightAxis = WristTransform.GetRotation().GetRightVector().GetSafeNormal();
		MotionControllerData.AimRotation = FRotationMatrix::MakeFromXY(AimDirection, AimRightAxis).ToQuat();
	}

	// Build grip pose from bones
	if (SimulationState->HasGripToWristTransform(Hand))
	{
		// Use stabilized grip transform while gripping
		const FTransform FrozenGripTransform = SimulationState->GetGripToWristTransform(Hand) * WristTransform;
		MotionControllerData.GripPosition = FrozenGripTransform.GetLocation();
		MotionControllerData.GripRotation = FrozenGripTransform.GetRotation();
	}
	else
	{
		MotionControllerData.GripPosition = PalmTransform.GetLocation();
		MotionControllerData.GripRotation = PalmTransform.GetRotation();
	}

	// Motion controller data only valid while tracking (consistent with physical devices)
	MotionControllerData.bValid = bIsTracked;
}

void AXRSimulationActor::GetControllerActionState(EControllerHand Hand, bool& OutSelectPressed, bool& OutGripPressed) const
{
	const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();
	check(Settings);

	OutSelectPressed = false;
	OutGripPressed = false;

	if (!SimulationState.IsValid())
	{
		return;
	}

	const FKey& SelectKey = (Hand == EControllerHand::Left ? FXRSimulationKeys::LeftSelect : FXRSimulationKeys::RightSelect);
	const FKey& GripKey = (Hand == EControllerHand::Left ? FXRSimulationKeys::LeftGrip : FXRSimulationKeys::RightGrip);

	FName TargetPose = SimulationState->GetTargetPose(Hand);
	for (const FXRSimulationHandPoseKeyMapping& Mapping : Settings->HandPoseKeys)
	{
		if (Mapping.Hand == Hand && Mapping.HandPose == TargetPose)
		{
			if (Mapping.Key == SelectKey)
			{
				OutSelectPressed = true;
			}
			if (Mapping.Key == GripKey)
			{
				OutGripPressed = true;
			}
		}
	}
}

void AXRSimulationActor::SetupHeadComponents()
{
	HeadMovement = CreateDefaultSubobject<UXRSimulationHeadMovementComponent>(TEXT("HeadMovement"));
	AddOwnedComponent(HeadMovement);
	// Add tick dependency so the head movement happens before the actor copies the result
	AddTickPrerequisiteComponent(HeadMovement);
}

void AXRSimulationActor::SetupHandComponents()
{
	LeftHand = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftHand"));
	AddOwnedComponent(LeftHand);
	LeftHand->SetMobility(EComponentMobility::Movable);
	LeftHand->SetupAttachment(RootComponent);

	RightHand = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RightHand"));
	AddOwnedComponent(RightHand);
	RightHand->SetMobility(EComponentMobility::Movable);
	RightHand->SetupAttachment(RootComponent);

	// Disable collisions
	LeftHand->SetCollisionProfileName(TEXT("NoCollision"));
	RightHand->SetCollisionProfileName(TEXT("NoCollision"));

	// Tag mesh components, so the anim BP knows what target pose to use.
	LeftHand->ComponentTags.Add(TEXT("Left"));
	RightHand->ComponentTags.Add(TEXT("Right"));

	// Add tick dependency so the hand animation happens before the actor copies the result
	AddTickPrerequisiteComponent(LeftHand);
	AddTickPrerequisiteComponent(RightHand);

	// Disable shadows
	LeftHand->SetCastShadow(false);
	RightHand->SetCastShadow(false);
}

void AXRSimulationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Note: using OnConstruction rather than BeginPlay for early initialization of simulation data,
	// to ensure it is valid when other actors request data for HMD, hand tracking, etc.

	// Initialize non-persistent data from simulation state
	{
		const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();

		// Head movement flag only gets initialized from XRSimulationRuntimeSettings, not simulated yet
		HeadMovement->SetHeadMovementEnabled(Settings->bStartWithPositionalHeadTracking);

		// Controller is used as the stage and the HMD moves relative to it
		if (SimulationState.IsValid())
		{
			HeadMovement->UpdatedComponent->SetRelativeLocation(SimulationState->RelativeHeadPosition);
			HeadMovement->UpdatedComponent->SetRelativeRotation(SimulationState->RelativeHeadOrientation);
		}
	}
}

void AXRSimulationActor::BeginPlay()
{
	Super::BeginPlay();

	const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();

	// Link the skeletal mesh components to animation assets.
	if (!Settings->HandMesh.IsNull())
	{
		USkeletalMesh* HandMesh = Settings->HandMesh.LoadSynchronous();
		LeftHand->SetSkeletalMesh(HandMesh);
		RightHand->SetSkeletalMesh(HandMesh);
	}
	if (!Settings->HandAnimBlueprint.IsNull())
	{
		UAnimBlueprint* AnimBP = Settings->HandAnimBlueprint.LoadSynchronous();
		if (AnimBP)
		{
			LeftHand->SetAnimInstanceClass(AnimBP->GeneratedClass);
			RightHand->SetAnimInstanceClass(AnimBP->GeneratedClass);
		}
	}

	if (ensure(InputComponent != nullptr))
	{
		if (bAddDefaultInputBindings)
		{
			BindInputEvents();
		}
	}
}

void AXRSimulationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterInputMappings();
}

void AXRSimulationActor::Tick(float DeltaSeconds)
{
	UpdateHandMeshComponent(EControllerHand::Left);
	UpdateHandMeshComponent(EControllerHand::Right);

	// Freeze the grip-to-wrist transform when gripping starts
	UpdateStabilizedGripTransform(EControllerHand::Left);
	UpdateStabilizedGripTransform(EControllerHand::Right);

	// Copy head pose to the simulation state for persistence
	if (SimulationState.IsValid())
	{
		SimulationState->RelativeHeadPosition = HeadMovement->UpdatedComponent->GetRelativeLocation();
		SimulationState->RelativeHeadOrientation = HeadMovement->UpdatedComponent->GetRelativeRotation().Quaternion();
	}
}

USkeletalMeshComponent* AXRSimulationActor::GetHandMesh(EControllerHand Hand) const
{
	if (Hand == EControllerHand::Left)
	{
		return LeftHand;
	}
	if (Hand == EControllerHand::Right)
	{
		return RightHand;
	}
	return nullptr;
}

void AXRSimulationActor::UpdateHandMeshComponent(EControllerHand Hand)
{
	if (USkeletalMeshComponent* HandMesh = GetHandMesh(Hand))
	{
		bool bVisible = SimulationState.IsValid() ? bVisible = SimulationState->IsHandVisible(Hand) : false;

		if (HandMesh->IsVisible() != bVisible)
		{
			HandMesh->SetVisibility(bVisible);
		}
	}
}

void AXRSimulationActor::RegisterEnhancedInputAxis(UInputAction*& Action, FText Description, TArray<FKey> Keys, bool Negate)
{
	Action = NewObject<UInputAction>();
	Action->AddToRoot();

	Action->ActionDescription = Description;
	Action->Triggers.Add(NewObject<UInputTriggerDown>());
	Action->bConsumeInput = false;
	Action->ValueType = EInputActionValueType::Axis1D;

	if (Negate)
	{
		Action->Modifiers.Add(NewObject<UInputModifierNegate>());
	}

	for (FKey Key : Keys)
	{
		InputMappingContext->MapKey(Action, Key);
	}
}

void AXRSimulationActor::RegisterEnhancedInputAction(UInputAction*& Action, FText Description, TArray<FKey> Keys)
{
	Action = NewObject<UInputAction>();
	Action->AddToRoot();

	Action->ActionDescription = Description;
	Action->Triggers.Add(NewObject<UInputTriggerDown>());
	Action->bConsumeInput = false;

	for (FKey Key : Keys)
	{
		InputMappingContext->MapKey(Action, Key);
	}
}

void AXRSimulationActor::RegisterInputMappings()
{
	// Axis mappings
	RegisterEnhancedInputAxis(
		MoveForward,
		FText::FromName(TEXT("XRSimulation_Axis_MoveForward")), 
		TArray<FKey> { EKeys::W, EKeys::Up, EKeys::Gamepad_LeftY }
	);

	RegisterEnhancedInputAxis(
		MoveBackward,
		FText::FromName(TEXT("XRSimulation_Axis_MoveBackward")),
		TArray<FKey> { EKeys::S, EKeys::Down },
		true
	);

	RegisterEnhancedInputAxis(
		MoveRight,
		FText::FromName(TEXT("XRSimulation_Axis_MoveRight")),
		TArray<FKey> { EKeys::D, EKeys::Gamepad_LeftX }
	);

	RegisterEnhancedInputAxis(
		MoveLeft,
		FText::FromName(TEXT("XRSimulation_Axis_MoveLeft")),
		TArray<FKey> { EKeys::A },
		true
	);

	RegisterEnhancedInputAxis(
		MoveUp,
		FText::FromName(TEXT("XRSimulation_Axis_MoveUp")),
		TArray<FKey> { EKeys::E, EKeys::Gamepad_FaceButton_Bottom, EKeys::Gamepad_LeftThumbstick }
	);

	RegisterEnhancedInputAxis(
		MoveDown,
		FText::FromName(TEXT("XRSimulation_Axis_MoveDown")),
		TArray<FKey> { EKeys::Q, EKeys::Gamepad_RightThumbstick },
		true
	);

	RegisterEnhancedInputAxis(
		Turn,
		FText::FromName(TEXT("XRSimulation_Axis_Turn")),
		TArray<FKey> { EKeys::MouseX }
	);

	RegisterEnhancedInputAxis(
		TurnRateRight,
		FText::FromName(TEXT("XRSimulation_Axis_TurnRateRight")),
		TArray<FKey> { EKeys::Right, EKeys::Gamepad_RightX }
	);

	RegisterEnhancedInputAxis(
		TurnRateLeft,
		FText::FromName(TEXT("XRSimulation_Axis_TurnRateLeft")),
		TArray<FKey> { EKeys::Left },
		true
	);

	RegisterEnhancedInputAxis(
		LookUp,
		FText::FromName(TEXT("XRSimulation_Axis_LookUp")),
		TArray<FKey> { EKeys::MouseY }
	);

	RegisterEnhancedInputAxis(
		LookUpRate,
		FText::FromName(TEXT("XRSimulation_Axis_LookUpRate")),
		TArray<FKey> { EKeys::Gamepad_RightY },
		true
	);

	RegisterEnhancedInputAxis(
		Scroll,
		FText::FromName(TEXT("XRSimulation_Axis_Scroll")),
		TArray<FKey> { EKeys::MouseWheelAxis }
	);

	// Action Mappings
	RegisterEnhancedInputAction(
		ToggleLeftHand,
		FText::FromName(TEXT("XRSimulation_Action_ToggleLeftHand")),
		TArray<FKey> { EKeys::T }
	);

	RegisterEnhancedInputAction(
		ToggleRightHand,
		FText::FromName(TEXT("XRSimulation_Action_ToggleRightHand")),
		TArray<FKey> { EKeys::Y }
	);

	RegisterEnhancedInputAction(
		ControlLeftHand,
		FText::FromName(TEXT("XRSimulation_Action_ControlLeftHand")),
		TArray<FKey> { EKeys::LeftShift }
	);

	RegisterEnhancedInputAction(
		ControlRightHand,
		FText::FromName(TEXT("XRSimulation_Action_ControlRightHand")),
		TArray<FKey> { EKeys::LeftAlt }
	);

	RegisterEnhancedInputAction(
		HandRotate,
		FText::FromName(TEXT("XRSimulation_Action_HandRotate")),
		TArray<FKey> { EKeys::RightMouseButton }
	);

	RegisterEnhancedInputAction(
		PrimaryHandPose,
		FText::FromName(TEXT("XRSimulation_Action_PrimaryHandPose")),
		TArray<FKey> { EKeys::LeftMouseButton }
	);

	RegisterEnhancedInputAction(
		SecondaryHandPose,
		FText::FromName(TEXT("XRSimulation_Action_SecondaryHandPose")),
		TArray<FKey> { EKeys::MiddleMouseButton }
	);

	RegisterEnhancedInputAction(
		MenuHandPose,
		FText::FromName(TEXT("XRSimulation_Action_MenuHandPose")),
		TArray<FKey> { EKeys::Home }
	);
}

void AXRSimulationActor::UnregisterInputMappings()
{
	InputMappingContext->UnmapAllKeysFromAction(MoveForward);
	InputMappingContext->UnmapAllKeysFromAction(MoveBackward);
	InputMappingContext->UnmapAllKeysFromAction(MoveRight);
	InputMappingContext->UnmapAllKeysFromAction(MoveLeft);
	InputMappingContext->UnmapAllKeysFromAction(MoveUp);
	InputMappingContext->UnmapAllKeysFromAction(MoveDown);
	InputMappingContext->UnmapAllKeysFromAction(Turn);
	InputMappingContext->UnmapAllKeysFromAction(TurnRateRight);
	InputMappingContext->UnmapAllKeysFromAction(TurnRateLeft);
	InputMappingContext->UnmapAllKeysFromAction(LookUp);
	InputMappingContext->UnmapAllKeysFromAction(LookUpRate);
	InputMappingContext->UnmapAllKeysFromAction(Scroll);

	InputMappingContext->UnmapAllKeysFromAction(ToggleLeftHand);
	InputMappingContext->UnmapAllKeysFromAction(ToggleRightHand);
	InputMappingContext->UnmapAllKeysFromAction(ControlLeftHand);
	InputMappingContext->UnmapAllKeysFromAction(ControlRightHand);
	InputMappingContext->UnmapAllKeysFromAction(HandRotate);
	InputMappingContext->UnmapAllKeysFromAction(PrimaryHandPose);
	InputMappingContext->UnmapAllKeysFromAction(SecondaryHandPose);
	InputMappingContext->UnmapAllKeysFromAction(MenuHandPose);
}

void AXRSimulationActor::BindInputEvents()
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(MoveForward, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputMoveForward);
		EnhancedInputComponent->BindAction(MoveBackward, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputMoveForward);
		EnhancedInputComponent->BindAction(MoveLeft, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputMoveRight);
		EnhancedInputComponent->BindAction(MoveRight, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputMoveRight);
		EnhancedInputComponent->BindAction(MoveUp, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputMoveUp);
		EnhancedInputComponent->BindAction(MoveDown, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputMoveUp);
		EnhancedInputComponent->BindAction(LookUp, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputLookUp);
		EnhancedInputComponent->BindAction(Turn, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputTurn);
		EnhancedInputComponent->BindAction(Scroll, ETriggerEvent::Triggered, this,
			&AXRSimulationActor::AddInputScroll);

		EnhancedInputComponent->BindAction(ToggleLeftHand, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnToggleLeftHandPressed);
		EnhancedInputComponent->BindAction(ToggleRightHand, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnToggleRightHandPressed);
		EnhancedInputComponent->BindAction(ControlLeftHand, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnControlLeftHandPressed);
		EnhancedInputComponent->BindAction(ControlLeftHand, ETriggerEvent::Completed, this,
			&AXRSimulationActor::OnControlLeftHandReleased);
		EnhancedInputComponent->BindAction(ControlRightHand, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnControlRightHandPressed);
		EnhancedInputComponent->BindAction(ControlRightHand, ETriggerEvent::Completed, this,
			&AXRSimulationActor::OnControlRightHandReleased);
		EnhancedInputComponent->BindAction(HandRotate, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnHandRotatePressed);
		EnhancedInputComponent->BindAction(HandRotate, ETriggerEvent::Completed, this,
			&AXRSimulationActor::OnHandRotateReleased);
		EnhancedInputComponent->BindAction(PrimaryHandPose, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnPrimaryHandPosePressed);
		EnhancedInputComponent->BindAction(SecondaryHandPose, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnSecondaryHandPosePressed);
		EnhancedInputComponent->BindAction(MenuHandPose, ETriggerEvent::Started, this,
			&AXRSimulationActor::OnMenuHandPosePressed);
	}
}

void AXRSimulationActor::OnToggleLeftHandPressed()
{
	if (SimulationState.IsValid())
	{
		SimulationState->SetHandVisibility(EControllerHand::Left, !SimulationState->IsHandVisible(EControllerHand::Left));
	}
}

void AXRSimulationActor::OnToggleRightHandPressed()
{
	if (SimulationState.IsValid())
	{
		SimulationState->SetHandVisibility(EControllerHand::Right, !SimulationState->IsHandVisible(EControllerHand::Right));
	}
}

void AXRSimulationActor::OnControlLeftHandPressed()
{
	if (SimulationState.IsValid())
	{
		SimulationState->SetHandControlEnabled(EControllerHand::Left, true);
	}
}

void AXRSimulationActor::OnControlLeftHandReleased()
{
	if (SimulationState.IsValid())
	{
		SimulationState->SetHandControlEnabled(EControllerHand::Left, false);
	}
}

void AXRSimulationActor::OnControlRightHandPressed()
{
	if (SimulationState.IsValid())
	{
		SimulationState->SetHandControlEnabled(EControllerHand::Right, true);
	}
}

void AXRSimulationActor::OnControlRightHandReleased()
{
	if (SimulationState.IsValid())
	{
		SimulationState->SetHandControlEnabled(EControllerHand::Right, false);
	}
}

void AXRSimulationActor::OnHandRotatePressed()
{
	if (SimulationState.IsValid())
	{
		SimulationState->HandInputMode = EXRSimulationHandMode::Rotation;
	}
}

void AXRSimulationActor::OnHandRotateReleased()
{
	if (SimulationState.IsValid())
	{
		SimulationState->HandInputMode = EXRSimulationHandMode::Movement;
	}
}

bool AXRSimulationActor::GetKeypointTransforms(
	EControllerHand Hand, const TArray<EHandKeypoint>& Keypoints, TArray<FTransform>& OutTransforms, TArray<float>& OutRadii) const
{
	const UEnum* KeypointEnum = StaticEnum<EHandKeypoint>();
	USkeletalMeshComponent* MeshComp = GetHandMesh(Hand);
	if (!ensureAsRuntimeWarning(MeshComp != nullptr))
	{
		return false;
	}

	TArray<FName> BoneNames;
	MeshComp->GetBoneNames(BoneNames);
	const TArray<FTransform>& ComponentSpaceTMs = MeshComp->GetComponentSpaceTransforms();

	OutTransforms.Reset(Keypoints.Num());
	OutRadii.Reset(Keypoints.Num());
	for (EHandKeypoint Keypoint : Keypoints)
	{
		FName KeypointName = FName(*KeypointEnum->GetNameStringByValue((int64)Keypoint));

		int32 KeypointPoseIndex;
		FTransform KeypointTransform;
		if (BoneNames.Find(KeypointName, KeypointPoseIndex))
		{
			KeypointTransform = ComponentSpaceTMs[KeypointPoseIndex];
		}
		else
		{
			KeypointTransform.SetIdentity();
		}

		FTransform::Multiply(&KeypointTransform, &KeypointTransform, &MeshComp->GetComponentTransform());

		// TODO What skeletal mesh property could be used for the radius?
		float KeypointRadius = 1.0f;

		OutTransforms.Add(KeypointTransform);
		OutRadii.Add(KeypointRadius);
	}

	return true;
}

void AXRSimulationActor::UpdateStabilizedGripTransform(EControllerHand Hand)
{
	if (SimulationState.IsValid())
	{
		bool bSelect, bGrip;
		GetControllerActionState(Hand, bSelect, bGrip);

		const bool bGripFrozen = SimulationState->HasGripToWristTransform(Hand);

		if (bGrip && !bGripFrozen)
		{
			// Freeze grip transform
			TArray<FTransform> KeypointTransforms;
			TArray<float> KeypointRadii;
			if (GetKeypointTransforms(Hand, {EHandKeypoint::Palm, EHandKeypoint::Wrist}, KeypointTransforms, KeypointRadii))
			{
				const FTransform& PalmTransform = KeypointTransforms[(int32)EHandKeypoint::Palm];
				const FTransform& WristTransform = KeypointTransforms[(int32)EHandKeypoint::Wrist];
				SimulationState->SetGripToWristTransform(Hand, PalmTransform.GetRelativeTransform(WristTransform));
			}
		}
		else if (!bGrip && bGripFrozen)
		{
			// Unfreeze grip transform
			SimulationState->ClearGripToWristTransform(Hand);
		}
	}
}

void AXRSimulationActor::OnPrimaryHandPosePressed()
{
	if (SimulationState.IsValid())
	{
		const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();
		check(Settings);
		SimulationState->TogglePoseForControlledHands(Settings->PrimaryHandPose);
	}
}

void AXRSimulationActor::OnSecondaryHandPosePressed()
{
	if (SimulationState.IsValid())
	{
		const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();
		check(Settings);
		SimulationState->TogglePoseForControlledHands(Settings->SecondaryHandPose);
	}
}

void AXRSimulationActor::OnMenuHandPosePressed()
{
	if (SimulationState.IsValid())
	{
		const UXRSimulationRuntimeSettings* const Settings = UXRSimulationRuntimeSettings::Get();
		check(Settings);
		SimulationState->TogglePoseForControlledHands(Settings->MenuHandPose);
	}
}

void AXRSimulationActor::AddInputMoveForward(const FInputActionInstance& ActionInstance)
{
	AddHeadMovementInputImpl(EAxis::X, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
}

void AXRSimulationActor::AddInputMoveRight(const FInputActionInstance& ActionInstance)
{
	AddHeadMovementInputImpl(EAxis::Y, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
}

void AXRSimulationActor::AddInputMoveUp(const FInputActionInstance& ActionInstance)
{
	AddHeadMovementInputImpl(EAxis::Z, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
}

void AXRSimulationActor::AddInputLookUp(const FInputActionInstance& ActionInstance)
{
	if (SimulationState.IsValid())
	{
		if (SimulationState->IsAnyHandControlled())
		{
			SimulationState->AddHandInput(EAxis::Z, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
		}
		else
		{
			AddHeadRotationInputImpl(EAxis::Z, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
		}
	}
}

void AXRSimulationActor::AddInputTurn(const FInputActionInstance& ActionInstance)
{
	if (SimulationState.IsValid())
	{
		if (SimulationState->IsAnyHandControlled())
		{
			SimulationState->AddHandInput(EAxis::Y, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
		}
		else
		{
			AddHeadRotationInputImpl(EAxis::Y, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>());
		}
	}
}

void AXRSimulationActor::AddInputScroll(const FInputActionInstance& ActionInstance)
{
	if (SimulationState.IsValid())
	{
		if (SimulationState->IsAnyHandControlled())
		{
			SimulationState->AddHandInput(EAxis::X, ActionInstance.GetValue().Get<FInputActionValue::Axis1D>() * 3);
		}
		else
		{
			// No head rotation on scrolling
		}
	}
}

void AXRSimulationActor::AddHeadMovementInputImpl(EAxis::Type Axis, float Value)
{
	FVector Dir = FRotationMatrix(GetActorRotation()).GetScaledAxis(Axis);
	HeadMovement->AddMovementInput(Dir * Value);
}

void AXRSimulationActor::AddHeadRotationInputImpl(EAxis::Type Axis, float Value)
{
	EAxis::Type RotationAxis = FXRInputAnimationUtils::GetInputRotationAxis(Axis);
	float RotationValue = FXRInputAnimationUtils::GetHeadRotationInputValue(RotationAxis, Value);
	FRotator RotationInput = FRotator::ZeroRotator;
	RotationInput.SetComponentForAxis(RotationAxis, RotationValue);
	HeadMovement->AddRotationInput(RotationInput);
}

#undef LOCTEXT_NAMESPACE
