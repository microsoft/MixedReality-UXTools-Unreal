// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtInputSimulationActor.h"
#include "UxtInputSimulationHeadMovementComponent.h"
#include "UxtRuntimeSettings.h"

#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"

#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerInput.h"
#include "Misc/RuntimeErrors.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "UXToolsInputSimulation"

AUxtInputSimulationActor::AUxtInputSimulationActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
	HeadMovement = CreateDefaultSubobject<UxtInputSimulationHeadMovementComponent>(TEXT("HeadMovement"));
	AddOwnedComponent(HeadMovement);
	// Add tick dependency so the head movement happens before the actor copies the result
	AddTickPrerequisiteComponent(HeadMovement);
}

void AUxtInputSimulationActor::SetupHandComponents()
{
	const auto* Settings = UUxtRuntimeSettings::Get();

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

	// Initial location
	LeftHand->SetRelativeLocation(FVector(40, -20, 0));
	RightHand->SetRelativeLocation(FVector(40, 20, 0));

	// Mirror the left hand.
	LeftHand->SetRelativeScale3D(FVector(1, -1, 1));
	RightHand->SetRelativeScale3D(FVector(1, 1, 1));

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
	const FName Action_ControlLeftHand = TEXT("InputSimulation_ControlLeftHand");
	const FName Action_ControlRightHand = TEXT("InputSimulation_ControlRightHand");
	const FName Action_PrimaryHandPose = TEXT("InputSimulation_PrimaryHandPose");
	const FName Action_SecondaryHandPose = TEXT("InputSimulation_SecondaryHandPose");
	const FName Axis_MoveForward = TEXT("InputSimulation_MoveForward");
	const FName Axis_MoveRight = TEXT("InputSimulation_MoveRight");
	const FName Axis_MoveUp = TEXT("InputSimulation_MoveUp");
	const FName Axis_Scroll = TEXT("InputSimulation_Scroll");
	const FName Axis_ScrollRate = TEXT("InputSimulation_ScrollRate");
	const FName Axis_Turn = TEXT("InputSimulation_Turn");
	const FName Axis_TurnRate = TEXT("InputSimulation_TurnRate");
	const FName Axis_LookUp = TEXT("InputSimulation_LookUp");
	const FName Axis_LookUpRate = TEXT("InputSimulation_LookUpRate");
}

static void InitializeDefaultInputSimulationMappings()
{
	static bool bMappingsAdded = false;
	if (!bMappingsAdded)
	{
		bMappingsAdded = true;

		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_ControlLeftHand, EKeys::LeftShift));
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_ControlRightHand, EKeys::LeftAlt));

		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_PrimaryHandPose, EKeys::LeftMouseButton));
		UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping(Action_SecondaryHandPose, EKeys::MiddleMouseButton));

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
		//UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(Axis_ScrollRate, ???, 1.f));
	}
}

void AUxtInputSimulationActor::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			// Attach to the player pawn so the start location matches the HMD camera, which is relative to the pawn.
			AttachToActor(Pawn, FAttachmentTransformRules::KeepRelativeTransform);

			// Hide the pawn for the local player
			Pawn->SetActorHiddenInGame(true);
		}
	}

	if (ensure(InputComponent != nullptr))
	{
		if (bAddDefaultInputBindings)
		{
			InitializeDefaultInputSimulationMappings();

			InputComponent->BindAction(Action_ControlLeftHand, IE_Pressed, this, &AUxtInputSimulationActor::OnControlLeftHandPressed);
			InputComponent->BindAction(Action_ControlLeftHand, IE_Released, this, &AUxtInputSimulationActor::OnControlLeftHandReleased);
			InputComponent->BindAction(Action_ControlRightHand, IE_Pressed, this, &AUxtInputSimulationActor::OnControlRightHandPressed);
			InputComponent->BindAction(Action_ControlRightHand, IE_Released, this, &AUxtInputSimulationActor::OnControlRightHandReleased);

			InputComponent->BindAction(Action_PrimaryHandPose, IE_Pressed, this, &AUxtInputSimulationActor::OnPrimaryHandPosePressed);
			InputComponent->BindAction(Action_PrimaryHandPose, IE_Released, this, &AUxtInputSimulationActor::OnPrimaryHandPoseReleased);
			InputComponent->BindAction(Action_SecondaryHandPose, IE_Pressed, this, &AUxtInputSimulationActor::OnSecondaryHandPosePressed);
			InputComponent->BindAction(Action_SecondaryHandPose, IE_Released, this, &AUxtInputSimulationActor::OnSecondaryHandPoseReleased);

			InputComponent->BindAxis(Axis_MoveForward, this, &AUxtInputSimulationActor::AddInputMoveForward);
			InputComponent->BindAxis(Axis_MoveRight, this, &AUxtInputSimulationActor::AddInputMoveRight);
			InputComponent->BindAxis(Axis_MoveUp, this, &AUxtInputSimulationActor::AddInputMoveUp);

			InputComponent->BindAxis(Axis_LookUp, this, &AUxtInputSimulationActor::AddInputLookUp);
			InputComponent->BindAxis(Axis_Turn, this, &AUxtInputSimulationActor::AddInputTurn);
			InputComponent->BindAxis(Axis_Scroll, this, &AUxtInputSimulationActor::AddInputScroll);
		}
	}
}

void AUxtInputSimulationActor::Tick(float DeltaSeconds)
{
	// Copy Simulated input data to the engine subsystem

	auto* InputSim = UWindowsMixedRealityInputSimulationEngineSubsystem::GetInputSimulationIfEnabled();
	if (!InputSim)
	{
		return;
	}

	// TODO Simulate this
	bool bHasPositionalTracking = true;

	FQuat HeadRotation = GetActorRotation().Quaternion();
	FVector HeadLocation = GetActorLocation();

	FWindowsMixedRealityInputSimulationHandState LeftHandState, RightHandState;
	CopySimulatedHandState(EControllerHand::Left, LeftHandState);
	CopySimulatedHandState(EControllerHand::Right, RightHandState);

	InputSim->UpdateSimulatedData(bHasPositionalTracking, HeadRotation, HeadLocation, LeftHandState, RightHandState);
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

FName AUxtInputSimulationActor::GetTargetPose(EControllerHand Hand) const
{
	const auto* Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	if (!ControlledHands.Contains(Hand))
	{
		return Settings->DefaultHandPose;
	}

	return TargetPoseStack.Num() > 0 ? TargetPoseStack.Last() : Settings->DefaultHandPose;
}

void AUxtInputSimulationActor::PushTargetPose(FName Name)
{
	if (TargetPoseStack.Num() > 0 && TargetPoseStack.Last() == Name)
	{
		// Already top pose, nothing to do
		return;
	}

	// Reinsert the pose at the top
	TargetPoseStack.Remove(Name);
	TargetPoseStack.Add(Name);
}

void AUxtInputSimulationActor::PopTargetPose(FName Name)
{
	TargetPoseStack.Remove(Name);
}

void AUxtInputSimulationActor::CopySimulatedHandState(EControllerHand Hand, FWindowsMixedRealityInputSimulationHandState& HandState) const
{
	typedef FWindowsMixedRealityInputSimulationHandState::ButtonStateArray ButtonStateArray;

	const auto* Settings = UUxtRuntimeSettings::Get();
	check(Settings);

	// TODO Simulate this
	bool IsTracking = true;
	HandState.TrackingStatus = IsTracking ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;

	// Copy joint poses from the bone transforms of the skeletal mesh if available
	USkeletalMeshComponent* MeshComp = GetHandMesh(Hand);
	if (!IsTracking || !MeshComp || !ensureAsRuntimeWarning(MeshComp != nullptr))
	{
		HandState.bHasJointPoses = false;
	}
	else
	{
		HandState.bHasJointPoses = true;

		const UEnum* KeypointEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EWMRHandKeypoint"), true);
		check(KeypointEnum);
		TArray<FName> BoneNames;
		MeshComp->GetBoneNames(BoneNames);

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
		}
	}

	// Update the button press states
	FName TargetPose = GetTargetPose(Hand);
	const EHMDInputControllerButtons* pTargetAction = Settings->HandPoseButtonMappings.Find(TargetPose);
	EHMDInputControllerButtons TargetAction = pTargetAction ? *pTargetAction : (EHMDInputControllerButtons)(-1);
	for (uint32 iButton = 0; iButton < (uint32)EHMDInputControllerButtons::Count; ++iButton)
	{
		EHMDInputControllerButtons Button = (EHMDInputControllerButtons)iButton;
		ButtonStateArray ButtonMask(true, iButton);
		if (Button == TargetAction)
		{
			HandState.IsButtonPressed |= ButtonMask;
		}
		else
		{
			HandState.IsButtonPressed &= ~ButtonMask;
		}
	}
}

void AUxtInputSimulationActor::OnControlLeftHandPressed()
{
	ControlledHands.Add(EControllerHand::Left);
}

void AUxtInputSimulationActor::OnControlLeftHandReleased()
{
	ControlledHands.Remove(EControllerHand::Left);
}

void AUxtInputSimulationActor::OnControlRightHandPressed()
{
	ControlledHands.Add(EControllerHand::Right);
}

void AUxtInputSimulationActor::OnControlRightHandReleased()
{
	ControlledHands.Remove(EControllerHand::Right);
}

void AUxtInputSimulationActor::OnPrimaryHandPosePressed()
{
	const auto* Settings = UUxtRuntimeSettings::Get();
	check(Settings);
	PushTargetPose(Settings->PrimaryHandPose);
}

void AUxtInputSimulationActor::OnPrimaryHandPoseReleased()
{
	const auto* Settings = UUxtRuntimeSettings::Get();
	check(Settings);
	PopTargetPose(Settings->PrimaryHandPose);
}

void AUxtInputSimulationActor::OnSecondaryHandPosePressed()
{
	const auto* Settings = UUxtRuntimeSettings::Get();
	check(Settings);
	PushTargetPose(Settings->SecondaryHandPose);
}

void AUxtInputSimulationActor::OnSecondaryHandPoseReleased()
{
	const auto* Settings = UUxtRuntimeSettings::Get();
	check(Settings);
	PopTargetPose(Settings->SecondaryHandPose);
}

void AUxtInputSimulationActor::AddInputMoveForward(float Value)
{
	AddMovementInputImpl(EAxis::X, Value);
}

void AUxtInputSimulationActor::AddInputMoveRight(float Value)
{
	AddMovementInputImpl(EAxis::Y, Value);
}

void AUxtInputSimulationActor::AddInputMoveUp(float Value)
{
	AddMovementInputImpl(EAxis::Z, Value);
}

const float InputYawScale = 2.5;
const float InputPitchScale = 1.75;

void AUxtInputSimulationActor::AddInputLookUp(float Value)
{
	if (ControlledHands.Num() > 0)
	{
		AddHandInputImpl(EAxis::Z, Value);
	}
	else
	{
		HeadMovement->AddRotationInput(FRotator(Value * InputPitchScale, 0, 0));
	}
}

void AUxtInputSimulationActor::AddInputTurn(float Value)
{
	if (ControlledHands.Num() > 0)
	{
		AddHandInputImpl(EAxis::Y, Value);
	}
	else
	{
		HeadMovement->AddRotationInput(FRotator(0, Value * InputYawScale, 0));
	}
}

void AUxtInputSimulationActor::AddInputScroll(float Value)
{
	if (ControlledHands.Num() > 0)
	{
		AddHandInputImpl(EAxis::X, Value);
	}
	else
	{
		// No rotation on scrolling
	}
}

void AUxtInputSimulationActor::AddMovementInputImpl(EAxis::Type Axis, float Value)
{
	FVector Dir = FRotationMatrix(GetActorRotation()).GetScaledAxis(Axis);
	HeadMovement->AddMovementInput(Dir * Value);
}

void AUxtInputSimulationActor::AddHandInputImpl(EAxis::Type Axis, float Value)
{
	if (Value != 0.f)
	{
		FVector Dir = FRotationMatrix(GetActorRotation()).GetScaledAxis(Axis);
		for (EControllerHand Hand : ControlledHands)
		{
			if (USkeletalMeshComponent* Comp = GetHandMesh(Hand))
			{

				Comp->MoveComponent(Dir * Value, Comp->GetComponentRotation(), true);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
