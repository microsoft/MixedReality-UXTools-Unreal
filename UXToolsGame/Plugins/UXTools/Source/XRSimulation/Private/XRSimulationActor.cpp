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
	const FName Action_ToggleLeftHand = TEXT("XRSimulation_Action_ToggleLeftHand");
	const FName Action_ToggleRightHand = TEXT("XRSimulation_Action_ToggleRightHand");

	const FName Action_ControlLeftHand = TEXT("XRSimulation_Action_ControlLeftHand");
	const FName Action_ControlRightHand = TEXT("XRSimulation_Action_ControlRightHand");

	const FName Action_HandRotate = TEXT("XRSimulation_Action_HandRotate");

	const FName Action_PrimaryHandPose = TEXT("XRSimulation_Action_PrimaryHandPose");
	const FName Action_SecondaryHandPose = TEXT("XRSimulation_Action_SecondaryHandPose");
	const FName Action_MenuHandPose = TEXT("XRSimulation_Action_MenuHandPose");

	const FName Axis_MoveForward = TEXT("XRSimulation_Axis_MoveForward");
	const FName Axis_MoveRight = TEXT("XRSimulation_Axis_MoveRight");
	const FName Axis_MoveUp = TEXT("XRSimulation_Axis_MoveUp");
	const FName Axis_Turn = TEXT("XRSimulation_Axis_Turn");
	const FName Axis_TurnRate = TEXT("XRSimulation_Axis_TurnRate");
	const FName Axis_LookUp = TEXT("XRSimulation_Axis_LookUp");
	const FName Axis_LookUpRate = TEXT("XRSimulation_Axis_LookUpRate");

	const FName Axis_Scroll = TEXT("XRSimulation_Axis_Scroll");
	const FName Axis_ScrollRate = TEXT("XRSimulation_Axis_ScrollRate");

	const TArray<FInputActionKeyMapping> ActionMappings({
		FInputActionKeyMapping(Action_ToggleLeftHand, EKeys::T),
		FInputActionKeyMapping(Action_ToggleRightHand, EKeys::Y),

		FInputActionKeyMapping(Action_ControlLeftHand, EKeys::LeftShift),
		FInputActionKeyMapping(Action_ControlRightHand, EKeys::LeftAlt),

		FInputActionKeyMapping(Action_HandRotate, EKeys::RightMouseButton),

		FInputActionKeyMapping(Action_PrimaryHandPose, EKeys::LeftMouseButton),
		FInputActionKeyMapping(Action_SecondaryHandPose, EKeys::MiddleMouseButton),
		FInputActionKeyMapping(Action_MenuHandPose, EKeys::Home),
	});

	const TArray<FInputAxisKeyMapping> AxisMappings({
		FInputAxisKeyMapping(Axis_MoveForward, EKeys::W, 1.f),
		FInputAxisKeyMapping(Axis_MoveForward, EKeys::S, -1.f),
		FInputAxisKeyMapping(Axis_MoveForward, EKeys::Up, 1.f),
		FInputAxisKeyMapping(Axis_MoveForward, EKeys::Down, -1.f),
		FInputAxisKeyMapping(Axis_MoveForward, EKeys::Gamepad_LeftY, 1.f),

		FInputAxisKeyMapping(Axis_MoveRight, EKeys::A, -1.f),
		FInputAxisKeyMapping(Axis_MoveRight, EKeys::D, 1.f),
		FInputAxisKeyMapping(Axis_MoveRight, EKeys::Gamepad_LeftX, 1.f),

		FInputAxisKeyMapping(Axis_MoveUp, EKeys::Gamepad_LeftThumbstick, 1.f),
		FInputAxisKeyMapping(Axis_MoveUp, EKeys::Gamepad_RightThumbstick, -1.f),
		FInputAxisKeyMapping(Axis_MoveUp, EKeys::Gamepad_FaceButton_Bottom, 1.f),
		FInputAxisKeyMapping(Axis_MoveUp, EKeys::E, 1.f),
		FInputAxisKeyMapping(Axis_MoveUp, EKeys::Q, -1.f),

		FInputAxisKeyMapping(Axis_Turn, EKeys::MouseX, 1.f),
		FInputAxisKeyMapping(Axis_TurnRate, EKeys::Gamepad_RightX, 1.f),
		FInputAxisKeyMapping(Axis_TurnRate, EKeys::Left, -1.f),
		FInputAxisKeyMapping(Axis_TurnRate, EKeys::Right, 1.f),

		FInputAxisKeyMapping(Axis_LookUp, EKeys::MouseY, 1.f),
		FInputAxisKeyMapping(Axis_LookUpRate, EKeys::Gamepad_RightY, -1.f),

		FInputAxisKeyMapping(Axis_Scroll, EKeys::MouseWheelAxis, 3.f),
		// FInputAxisKeyMapping(Axis_ScrollRate, ???, 1.f),
	});

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

void AXRSimulationActor::RegisterInputMappings()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		UE_LOG(LogXRSimulationActor, Warning, TEXT("Could not find mutable input settings"));
		return;
	}

	for (const FInputActionKeyMapping& Mapping : ActionMappings)
	{
		if (Mapping.Key.IsValid())
		{
			InputSettings->AddActionMapping(Mapping, false);
		}
	}
	for (const FInputAxisKeyMapping& Mapping : AxisMappings)
	{
		if (Mapping.Key.IsValid())
		{
			InputSettings->AddAxisMapping(Mapping, false);
		}
	}
	InputSettings->ForceRebuildKeymaps();
}

void AXRSimulationActor::UnregisterInputMappings()
{
	UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
	if (!InputSettings)
	{
		return;
	}

	for (const FInputActionKeyMapping& Mapping : ActionMappings)
	{
		InputSettings->RemoveActionMapping(Mapping, false);
	}
	for (const FInputAxisKeyMapping& Mapping : AxisMappings)
	{
		InputSettings->RemoveAxisMapping(Mapping, false);
	}
	InputSettings->ForceRebuildKeymaps();
}

void AXRSimulationActor::BindInputEvents()
{
	InputComponent->BindAction(Action_ToggleLeftHand, IE_Pressed, this, &AXRSimulationActor::OnToggleLeftHandPressed);
	InputComponent->BindAction(Action_ToggleRightHand, IE_Pressed, this, &AXRSimulationActor::OnToggleRightHandPressed);

	InputComponent->BindAction(Action_ControlLeftHand, IE_Pressed, this, &AXRSimulationActor::OnControlLeftHandPressed);
	InputComponent->BindAction(Action_ControlLeftHand, IE_Released, this, &AXRSimulationActor::OnControlLeftHandReleased);
	InputComponent->BindAction(Action_ControlRightHand, IE_Pressed, this, &AXRSimulationActor::OnControlRightHandPressed);
	InputComponent->BindAction(Action_ControlRightHand, IE_Released, this, &AXRSimulationActor::OnControlRightHandReleased);

	InputComponent->BindAction(Action_HandRotate, IE_Pressed, this, &AXRSimulationActor::OnHandRotatePressed);
	InputComponent->BindAction(Action_HandRotate, IE_Released, this, &AXRSimulationActor::OnHandRotateReleased);

	InputComponent->BindAction(Action_PrimaryHandPose, IE_Pressed, this, &AXRSimulationActor::OnPrimaryHandPosePressed);
	InputComponent->BindAction(Action_SecondaryHandPose, IE_Pressed, this, &AXRSimulationActor::OnSecondaryHandPosePressed);
	InputComponent->BindAction(Action_MenuHandPose, IE_Pressed, this, &AXRSimulationActor::OnMenuHandPosePressed);

	InputComponent->BindAxis(Axis_MoveForward, this, &AXRSimulationActor::AddInputMoveForward);
	InputComponent->BindAxis(Axis_MoveRight, this, &AXRSimulationActor::AddInputMoveRight);
	InputComponent->BindAxis(Axis_MoveUp, this, &AXRSimulationActor::AddInputMoveUp);

	InputComponent->BindAxis(Axis_LookUp, this, &AXRSimulationActor::AddInputLookUp);
	InputComponent->BindAxis(Axis_Turn, this, &AXRSimulationActor::AddInputTurn);
	InputComponent->BindAxis(Axis_Scroll, this, &AXRSimulationActor::AddInputScroll);
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

void AXRSimulationActor::AddInputMoveForward(float Value)
{
	AddHeadMovementInputImpl(EAxis::X, Value);
}

void AXRSimulationActor::AddInputMoveRight(float Value)
{
	AddHeadMovementInputImpl(EAxis::Y, Value);
}

void AXRSimulationActor::AddInputMoveUp(float Value)
{
	AddHeadMovementInputImpl(EAxis::Z, Value);
}

void AXRSimulationActor::AddInputLookUp(float Value)
{
	if (SimulationState.IsValid())
	{
		if (SimulationState->IsAnyHandControlled())
		{
			SimulationState->AddHandInput(EAxis::Z, Value);
		}
		else
		{
			AddHeadRotationInputImpl(EAxis::Z, Value);
		}
	}
}

void AXRSimulationActor::AddInputTurn(float Value)
{
	if (SimulationState.IsValid())
	{
		if (SimulationState->IsAnyHandControlled())
		{
			SimulationState->AddHandInput(EAxis::Y, Value);
		}
		else
		{
			AddHeadRotationInputImpl(EAxis::Y, Value);
		}
	}
}

void AXRSimulationActor::AddInputScroll(float Value)
{
	if (SimulationState.IsValid())
	{
		if (SimulationState->IsAnyHandControlled())
		{
			SimulationState->AddHandInput(EAxis::X, Value);
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
