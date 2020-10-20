// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationActor.h"

#include "UxtInputSimulationHeadMovementComponent.h"
#include "UxtInputSimulationLocalPlayerSubsystem.h"
#include "UxtRuntimeSettings.h"
#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"

#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/RuntimeErrors.h"

#define LOCTEXT_NAMESPACE "UXToolsInputSimulation"

AUxtInputSimulationActor::AUxtInputSimulationActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Tick after updates to copy data to the input simulation subsystem
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

void AUxtInputSimulationActor::SetupHeadComponents()
{
	HeadMovement = CreateDefaultSubobject<UUxtInputSimulationHeadMovementComponent>(TEXT("HeadMovement"));
	AddOwnedComponent(HeadMovement);
	// Add tick dependency so the head movement happens before the actor copies the result
	AddTickPrerequisiteComponent(HeadMovement);
}

void AUxtInputSimulationActor::SetupHandComponents()
{
	const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();

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

	// Link the skeletal mesh components to animation assets.
	if (Settings->HandMesh)
	{
		LeftHand->SetSkeletalMesh(Settings->HandMesh.Get());
		RightHand->SetSkeletalMesh(Settings->HandMesh.Get());
	}
	if (Settings->HandAnimInstance)
	{
		LeftHand->SetAnimInstanceClass(Settings->HandAnimInstance.Get());
		RightHand->SetAnimInstanceClass(Settings->HandAnimInstance.Get());
	}

	// Disable shadows
	LeftHand->SetCastShadow(false);
	RightHand->SetCastShadow(false);
}

namespace
{
	const FName Action_ToggleLeftHand = TEXT("InputSimulation_ToggleLeftHand");
	const FName Action_ToggleRightHand = TEXT("InputSimulation_ToggleRightHand");
	const FName Action_ControlLeftHand = TEXT("InputSimulation_ControlLeftHand");
	const FName Action_ControlRightHand = TEXT("InputSimulation_ControlRightHand");
	const FName Action_HandRotate = TEXT("InputSimulation_HandRotate");
	const FName Action_PrimaryHandPose = TEXT("InputSimulation_PrimaryHandPose");
	const FName Action_SecondaryHandPose = TEXT("InputSimulation_SecondaryHandPose");
	const FName Action_MenuHandPose = TEXT("InputSimulation_MenuHandPose");
	const FName Axis_MoveForward = TEXT("InputSimulation_MoveForward");
	const FName Axis_MoveRight = TEXT("InputSimulation_MoveRight");
	const FName Axis_MoveUp = TEXT("InputSimulation_MoveUp");
	const FName Axis_Scroll = TEXT("InputSimulation_Scroll");
	const FName Axis_ScrollRate = TEXT("InputSimulation_ScrollRate");
	const FName Axis_Turn = TEXT("InputSimulation_Turn");
	const FName Axis_TurnRate = TEXT("InputSimulation_TurnRate");
	const FName Axis_LookUp = TEXT("InputSimulation_LookUp");
	const FName Axis_LookUpRate = TEXT("InputSimulation_LookUpRate");
} // namespace

void AUxtInputSimulationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Note: using OnConstruction rather than BeginPlay for early initialization of simulation data,
	// to ensure it is valid when other actors request data for HMD, hand tracking, etc.

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		// Cache persistent simulation state for quick access
		SimulationStateWeak.Reset();
		if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PC->Player))
		{
			if (UUxtInputSimulationLocalPlayerSubsystem* InputSim = LocalPlayer->GetSubsystem<UUxtInputSimulationLocalPlayerSubsystem>())
			{
				SimulationStateWeak = InputSim->GetSimulationState();
			}
		}

		// Explicitly enable input: The input sim actor may be created after loading a map,
		// in which case auto-enabling input does not work.
		EnableInput(PC);

		// Attach to the player controller so the start location matches the HMD camera, which is relative to the controller.
		AttachToActor(PC, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// Initialize non-persistent data from simulation state
	if (const UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();

		// Head movement flag only gets initialized from UxtRuntimeSettings, not simulated yet
		HeadMovement->SetHeadMovementEnabled(Settings->bStartWithPositionalHeadTracking);

		// Controller is used as the stage and the HMD moves relative to it
		HeadMovement->UpdatedComponent->SetRelativeLocation(State->RelativeHeadPosition);
		HeadMovement->UpdatedComponent->SetRelativeRotation(State->RelativeHeadOrientation);
	}

	// Make sure device data is updated once before the first tick
	UpdateSimulatedDeviceData();
}

void AUxtInputSimulationActor::BeginPlay()
{
	Super::BeginPlay();

	// Hide the pawn for the local player
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			Pawn->SetActorHiddenInGame(true);
			Pawn->SetActorEnableCollision(false);
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

void AUxtInputSimulationActor::Tick(float DeltaSeconds)
{
	UpdateHandMeshComponent(EControllerHand::Left);
	UpdateHandMeshComponent(EControllerHand::Right);

	// Copy head pose to the simulation state for persistence
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->RelativeHeadPosition = HeadMovement->UpdatedComponent->GetRelativeLocation();
		State->RelativeHeadOrientation = HeadMovement->UpdatedComponent->GetRelativeRotation().Quaternion();
	}

	UpdateSimulatedDeviceData();
}

USkeletalMeshComponent* AUxtInputSimulationActor::GetHandMesh(EControllerHand Hand) const
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

void AUxtInputSimulationActor::UpdateHandMeshComponent(EControllerHand Hand)
{
	if (USkeletalMeshComponent* HandMesh = GetHandMesh(Hand))
	{
		bool bVisible;
		if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
		{
			bVisible = State->IsHandVisible(Hand);
		}
		else
		{
			bVisible = false;
		}

		if (HandMesh->IsVisible() != bVisible)
		{
			HandMesh->SetVisibility(bVisible);
		}
	}
}

void AUxtInputSimulationActor::UpdateSimulatedHandState(EControllerHand Hand, FWindowsMixedRealityInputSimulationHandState& HandState) const
{
	typedef FWindowsMixedRealityInputSimulationHandState::ButtonStateArray ButtonStateArray;

	UUxtInputSimulationState* State = SimulationStateWeak.Get();
	if (!State)
	{
		return;
	}

	const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
	check(Settings);
	USkeletalMeshComponent* MeshComp = GetHandMesh(Hand);

	// Visible hands are considered tracked
	bool IsTracked = State->IsHandVisible(Hand);

	HandState.TrackingStatus = IsTracked ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;

	// Toggle hand mesh visibility based on simulated tracking state
	if (MeshComp)
	{
		MeshComp->SetVisibility(IsTracked);
	}

	// Copy joint poses from the bone transforms of the skeletal mesh if available
	if (!IsTracked || !MeshComp || !ensureAsRuntimeWarning(MeshComp != nullptr))
	{
		HandState.bHasJointPoses = false;
		HandState.bHasPointerPose = false;
	}
	else
	{
		HandState.bHasJointPoses = true;

		const UEnum* KeypointEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EWMRHandKeypoint"), true);
		check(KeypointEnum);
		TArray<FName> BoneNames;
		MeshComp->GetBoneNames(BoneNames);

		// Transforms for pointer pose
		FTransform WristTransform = FTransform::Identity;
		FTransform IndexKnuckleTransform = FTransform::Identity;

		const TArray<FTransform>& ComponentSpaceTMs = MeshComp->GetComponentSpaceTransforms();
		for (int32 iKeypoint = 0; iKeypoint < EWMRHandKeypointCount; ++iKeypoint)
		{
			EWMRHandKeypoint Keypoint = (EWMRHandKeypoint)iKeypoint;
			FName KeypointName = FName(*KeypointEnum->GetNameStringByValue(iKeypoint));

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

			HandState.KeypointTransforms[(uint32)Keypoint] = KeypointTransform;
			HandState.KeypointRadii[(uint32)Keypoint] = KeypointRadius;

			if (Keypoint == EWMRHandKeypoint::Wrist)
			{
				WristTransform = KeypointTransform;
			}
			if (Keypoint == EWMRHandKeypoint::IndexProximal)
			{
				IndexKnuckleTransform = KeypointTransform;
			}
		}

		// Build pointer pose from bones
		{
			HandState.bHasPointerPose = true;

			USceneComponent* ParentComp = MeshComp->GetAttachParent();
			FVector ShoulderPos =
				(Hand == EControllerHand::Left ? Settings->ShoulderPosition.MirrorByVector(FVector::RightVector)
											   : Settings->ShoulderPosition);
			if (ParentComp)
			{
				ShoulderPos = ParentComp->GetComponentTransform().TransformPosition(ShoulderPos);
			}

			HandState.PointerPose.Origin = IndexKnuckleTransform.GetLocation();

			HandState.PointerPose.Direction = (MeshComp->GetComponentLocation() - ShoulderPos);
			HandState.PointerPose.Direction.Normalize();

			FVector HandAxis = WristTransform.GetRotation().GetRightVector();
			HandAxis.Normalize();
			HandState.PointerPose.Up = FVector::CrossProduct(HandState.PointerPose.Direction, HandAxis);
			HandState.PointerPose.Up.Normalize();

			HandState.PointerPose.Orientation = FRotationMatrix::MakeFromXY(HandState.PointerPose.Direction, HandAxis).ToQuat();
		}
	}

	//
	// Update the button press states

	HandState.IsButtonPressed = 0;

	FName TargetPose = State->GetTargetPose(Hand);
	const FUxtRuntimeSettingsButtonSet* pTargetActions = Settings->HandPoseButtonMappings.Find(TargetPose);
	if (pTargetActions)
	{
		for (uint32 iButton = 0; iButton < (uint32)EHMDInputControllerButtons::Count; ++iButton)
		{
			EHMDInputControllerButtons Button = (EHMDInputControllerButtons)iButton;
			ButtonStateArray ButtonMask(true, iButton);
			if (pTargetActions->Buttons.Contains(Button))
			{
				HandState.IsButtonPressed |= ButtonMask;
			}
		}
	}
	else
	{
		HandState.IsButtonPressed = 0;
	}
}

void AUxtInputSimulationActor::UpdateSimulatedDeviceData() const
{
	UWindowsMixedRealityInputSimulationEngineSubsystem* InputSim =
		UWindowsMixedRealityInputSimulationEngineSubsystem::GetInputSimulationIfEnabled();
	if (!InputSim)
	{
		return;
	}

	// Actor movement is used directly as HMD pose
	bool bHasPositionalTracking = HeadMovement->IsHeadMovementEnabled();
	FQuat HeadOrientation = HeadMovement->UpdatedComponent->GetComponentRotation().Quaternion();
	FVector HeadPosition = HeadMovement->UpdatedComponent->GetComponentLocation();

	// Construct new hand state from animation
	FWindowsMixedRealityInputSimulationHandState LeftHandState, RightHandState;
	UpdateSimulatedHandState(EControllerHand::Left, LeftHandState);
	UpdateSimulatedHandState(EControllerHand::Right, RightHandState);

	// Copy simulated input data to the engine subsystem
	InputSim->UpdateSimulatedData(bHasPositionalTracking, HeadOrientation, HeadPosition, LeftHandState, RightHandState);
}

static void InitializeDefaultInputSimulationMappings()
{
	static bool bMappingsAdded = false;
	if (!bMappingsAdded)
	{
		bMappingsAdded = true;

		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_ToggleLeftHand, EKeys::T));
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_ToggleRightHand, EKeys::Y));

		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_ControlLeftHand, EKeys::LeftShift));
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_ControlRightHand, EKeys::LeftAlt));

		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_HandRotate, EKeys::RightMouseButton));

		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_PrimaryHandPose, EKeys::LeftMouseButton));
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_SecondaryHandPose, EKeys::MiddleMouseButton));
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_MenuHandPose, EKeys::Home));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveForward, EKeys::W, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveForward, EKeys::S, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveForward, EKeys::Up, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveForward, EKeys::Down, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveForward, EKeys::Gamepad_LeftY, 1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveRight, EKeys::A, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveRight, EKeys::D, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveRight, EKeys::Gamepad_LeftX, 1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::Gamepad_LeftThumbstick, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::Gamepad_RightThumbstick, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::Gamepad_FaceButton_Bottom, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::SpaceBar, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::LeftControl, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::C, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::E, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_MoveUp, EKeys::Q, -1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_Turn, EKeys::MouseX, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_TurnRate, EKeys::Gamepad_RightX, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_TurnRate, EKeys::Left, -1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_TurnRate, EKeys::Right, 1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_LookUp, EKeys::MouseY, 1.f));
		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_LookUpRate, EKeys::Gamepad_RightY, -1.f));

		UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_Scroll, EKeys::MouseWheelAxis, 3.f));
		// UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_ScrollRate, ???, 1.f));
	}
}

void AUxtInputSimulationActor::BindInputEvents()
{
	InitializeDefaultInputSimulationMappings();

	InputComponent->BindAction(Action_ToggleLeftHand, IE_Pressed, this, &AUxtInputSimulationActor::OnToggleLeftHandPressed);
	InputComponent->BindAction(Action_ToggleRightHand, IE_Pressed, this, &AUxtInputSimulationActor::OnToggleRightHandPressed);

	InputComponent->BindAction(Action_ControlLeftHand, IE_Pressed, this, &AUxtInputSimulationActor::OnControlLeftHandPressed);
	InputComponent->BindAction(Action_ControlLeftHand, IE_Released, this, &AUxtInputSimulationActor::OnControlLeftHandReleased);
	InputComponent->BindAction(Action_ControlRightHand, IE_Pressed, this, &AUxtInputSimulationActor::OnControlRightHandPressed);
	InputComponent->BindAction(Action_ControlRightHand, IE_Released, this, &AUxtInputSimulationActor::OnControlRightHandReleased);

	InputComponent->BindAction(Action_HandRotate, IE_Pressed, this, &AUxtInputSimulationActor::OnHandRotatePressed);
	InputComponent->BindAction(Action_HandRotate, IE_Released, this, &AUxtInputSimulationActor::OnHandRotateReleased);

	InputComponent->BindAction(Action_PrimaryHandPose, IE_Pressed, this, &AUxtInputSimulationActor::OnPrimaryHandPosePressed);
	InputComponent->BindAction(Action_SecondaryHandPose, IE_Pressed, this, &AUxtInputSimulationActor::OnSecondaryHandPosePressed);
	InputComponent->BindAction(Action_MenuHandPose, IE_Pressed, this, &AUxtInputSimulationActor::OnMenuHandPosePressed);

	InputComponent->BindAxis(Axis_MoveForward, this, &AUxtInputSimulationActor::AddInputMoveForward);
	InputComponent->BindAxis(Axis_MoveRight, this, &AUxtInputSimulationActor::AddInputMoveRight);
	InputComponent->BindAxis(Axis_MoveUp, this, &AUxtInputSimulationActor::AddInputMoveUp);

	InputComponent->BindAxis(Axis_LookUp, this, &AUxtInputSimulationActor::AddInputLookUp);
	InputComponent->BindAxis(Axis_Turn, this, &AUxtInputSimulationActor::AddInputTurn);
	InputComponent->BindAxis(Axis_Scroll, this, &AUxtInputSimulationActor::AddInputScroll);
}

void AUxtInputSimulationActor::OnToggleLeftHandPressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->SetHandVisibility(EControllerHand::Left, !State->IsHandVisible(EControllerHand::Left));
	}
}

void AUxtInputSimulationActor::OnToggleRightHandPressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->SetHandVisibility(EControllerHand::Right, !State->IsHandVisible(EControllerHand::Right));
	}
}

void AUxtInputSimulationActor::OnControlLeftHandPressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->SetHandControlEnabled(EControllerHand::Left, true);
	}
}

void AUxtInputSimulationActor::OnControlLeftHandReleased()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->SetHandControlEnabled(EControllerHand::Left, false);
	}
}

void AUxtInputSimulationActor::OnControlRightHandPressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->SetHandControlEnabled(EControllerHand::Right, true);
	}
}

void AUxtInputSimulationActor::OnControlRightHandReleased()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->SetHandControlEnabled(EControllerHand::Right, false);
	}
}

void AUxtInputSimulationActor::OnHandRotatePressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->HandInputMode = EUxtInputSimulationHandMode::Rotation;
	}
}

void AUxtInputSimulationActor::OnHandRotateReleased()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		State->HandInputMode = EUxtInputSimulationHandMode::Movement;
	}
}

void AUxtInputSimulationActor::OnPrimaryHandPosePressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
		check(Settings);
		State->TogglePoseForControlledHands(Settings->PrimaryHandPose);
	}
}

void AUxtInputSimulationActor::OnSecondaryHandPosePressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
		check(Settings);
		State->TogglePoseForControlledHands(Settings->SecondaryHandPose);
	}
}

void AUxtInputSimulationActor::OnMenuHandPosePressed()
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		const UUxtRuntimeSettings* const Settings = UUxtRuntimeSettings::Get();
		check(Settings);
		State->TogglePoseForControlledHands(Settings->MenuHandPose);
	}
}

void AUxtInputSimulationActor::AddInputMoveForward(float Value)
{
	AddHeadMovementInputImpl(EAxis::X, Value);
}

void AUxtInputSimulationActor::AddInputMoveRight(float Value)
{
	AddHeadMovementInputImpl(EAxis::Y, Value);
}

void AUxtInputSimulationActor::AddInputMoveUp(float Value)
{
	AddHeadMovementInputImpl(EAxis::Z, Value);
}

void AUxtInputSimulationActor::AddInputLookUp(float Value)
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		if (State->IsAnyHandControlled())
		{
			State->AddHandInput(EAxis::Z, Value);
		}
		else
		{
			AddHeadRotationInputImpl(EAxis::Z, Value);
		}
	}
}

void AUxtInputSimulationActor::AddInputTurn(float Value)
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		if (State->IsAnyHandControlled())
		{
			State->AddHandInput(EAxis::Y, Value);
		}
		else
		{
			AddHeadRotationInputImpl(EAxis::Y, Value);
		}
	}
}

void AUxtInputSimulationActor::AddInputScroll(float Value)
{
	if (UUxtInputSimulationState* State = SimulationStateWeak.Get())
	{
		if (State->IsAnyHandControlled())
		{
			State->AddHandInput(EAxis::X, Value);
		}
		else
		{
			// No head rotation on scrolling
		}
	}
}

void AUxtInputSimulationActor::AddHeadMovementInputImpl(EAxis::Type Axis, float Value)
{
	FVector Dir = FRotationMatrix(GetActorRotation()).GetScaledAxis(Axis);
	HeadMovement->AddMovementInput(Dir * Value);
}

void AUxtInputSimulationActor::AddHeadRotationInputImpl(EAxis::Type Axis, float Value)
{
	EAxis::Type RotationAxis = FUxtInputAnimationUtils::GetInputRotationAxis(Axis);
	float RotationValue = FUxtInputAnimationUtils::GetHeadRotationInputValue(RotationAxis, Value);
	FRotator RotationInput = FRotator::ZeroRotator;
	RotationInput.SetComponentForAxis(RotationAxis, RotationValue);
	HeadMovement->AddRotationInput(RotationInput);
}

#undef LOCTEXT_NAMESPACE
