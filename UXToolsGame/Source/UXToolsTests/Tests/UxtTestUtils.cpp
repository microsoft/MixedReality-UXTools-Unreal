// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtTestUtils.h"

#include "Engine.h"
#include "EngineUtils.h"
#include "UxtTestHandTracker.h"
#include "UxtTestTargetComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Controls/UxtFarBeamComponent.h"
#include "Controls/UxtFarCursorComponent.h"
#include "Features/IModularFeatures.h"
#include "GameFramework/Actor.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"
#include "Tests/AutomationCommon.h"
#include "Utils/UxtFunctionLibrary.h"

namespace
{
	AActor* CreateNearPointerTargetActor(UWorld* World, const FVector& Location, const FString& MeshFilename, float MeshScale)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocation(Location);
		Root->SetWorldRotation(FRotator(0, 180, 0));
		Root->RegisterComponent();

		if (!MeshFilename.IsEmpty())
		{
			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor);
			Mesh->SetupAttachment(Actor->GetRootComponent());
			Mesh->Rename(TEXT("TargetSM"));
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Mesh->SetCollisionProfileName(TEXT("UI"));
			Mesh->SetGenerateOverlapEvents(true);

			UStaticMesh* MeshAsset = LoadObject<UStaticMesh>(Actor, *MeshFilename);
			Mesh->SetStaticMesh(MeshAsset);
			Mesh->SetRelativeScale3D(FVector::OneVector * MeshScale);

			Mesh->RegisterComponent();
		}

		return Actor;
	}
} // namespace

FUxtTestHandTracker UxtTestUtils::TestHandTracker;

/** Cached hand tracker implementation to restore after tests are completed. */
IUxtHandTracker* UxtTestUtils::MainHandTracker = nullptr;

UWorld* UxtTestUtils::LoadMap(const FString& MapName)
{
	// Syncronous map load is only supported in editor
#if WITH_EDITOR
	if (AutomationOpenMap(MapName))
	{
		return GetTestWorld();
	}
#else
	check(false);
#endif
	return nullptr;
}

void UxtTestUtils::ExitGame()
{
	// Copied from FExitGameCommand
	if (APlayerController* TargetPC = UGameplayStatics::GetPlayerController(GetTestWorld(), 0))
	{
		TargetPC->ConsoleCommand(TEXT("Exit"), true);
	}
}

// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
// Marked as temporary there, hence, this one is temporary, too.
UWorld* UxtTestUtils::GetTestWorld()
{
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if (((Context.WorldType == EWorldType::PIE) || (Context.WorldType == EWorldType::Game)) && (Context.World() != nullptr))
		{
			return Context.World();
		}
	}

	return nullptr;
}

UWorld* UxtTestUtils::CreateTestWorld()
{
	UWorld* world = UWorld::CreateWorld(EWorldType::PIE, true, TEXT("TestWorld"));
	return world;
}

UStaticMeshComponent* UxtTestUtils::CreateBoxStaticMesh(AActor* Owner, FVector Scale)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner);
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(Owner, TEXT("/Engine/BasicShapes/Cube.Cube"));
	Component->SetStaticMesh(Mesh);
	Component->SetWorldScale3D(Scale);
	return Component;
}

FUxtTestHandTracker& UxtTestUtils::GetTestHandTracker()
{
	return TestHandTracker;
}

FUxtTestHandTracker& UxtTestUtils::EnableTestHandTracker()
{
	check(MainHandTracker == nullptr);

	// Remove and cache current hand tracker.
	MainHandTracker = &IModularFeatures::Get().GetModularFeature<IUxtHandTracker>(IUxtHandTracker::GetModularFeatureName());
	if (MainHandTracker)
	{
		IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), MainHandTracker);
	}

	// Register the test hand tracker.
	IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &TestHandTracker);

	// Reset test hand tracker defaults
	TestHandTracker = FUxtTestHandTracker();

	return TestHandTracker;
}

void UxtTestUtils::DisableTestHandTracker()
{
	// Unregister the test hand tracker.
	IModularFeatures::Get().UnregisterModularFeature(IUxtHandTracker::GetModularFeatureName(), &TestHandTracker);

	// Re-register the original hand tracker implementation
	if (MainHandTracker)
	{
		IModularFeatures::Get().RegisterModularFeature(IUxtHandTracker::GetModularFeatureName(), MainHandTracker);
		MainHandTracker = nullptr;
	}
}

UUxtNearPointerComponent* UxtTestUtils::CreateNearPointer(
	UWorld* World, FName Name, const FVector& Location, EControllerHand Hand, bool IsGrasped, bool AddMeshVisualizer)
{
	FActorSpawnParameters ActorParams;
	ActorParams.Name = Name;
	ActorParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	AActor* PointerActor = World->SpawnActor<AActor>(ActorParams);

	USceneComponent* Root = NewObject<USceneComponent>(PointerActor);
	PointerActor->SetRootComponent(Root);
	Root->SetWorldLocation(Location);
	Root->RegisterComponent();

	UUxtNearPointerComponent* Pointer = NewObject<UUxtNearPointerComponent>(PointerActor);
	Pointer->RegisterComponent();
	Pointer->Hand = Hand;

	UxtTestUtils::GetTestHandTracker().SetGrabbing(IsGrasped);
	UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Location);

	if (AddMeshVisualizer)
	{
		UStaticMeshComponent* Mesh = CreateBoxStaticMesh(PointerActor, FVector(0.01f));
		Mesh->SetupAttachment(Root);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->RegisterComponent();
	}

	return Pointer;
}

UUxtFarPointerComponent* UxtTestUtils::CreateFarPointer(
	UWorld* World, FName Name, const FVector& Position, EControllerHand Hand, bool IsGrasped)
{
	UxtTestUtils::GetTestHandTracker().SetAllJointPositions(Position, Hand);
	UxtTestUtils::GetTestHandTracker().SetAllJointOrientations(FQuat::Identity, Hand);
	UxtTestUtils::GetTestHandTracker().SetGrabbing(IsGrasped, Hand);

	FActorSpawnParameters ActorParams;
	ActorParams.Name = Name;
	ActorParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
	AActor* PointerActor = World->SpawnActor<AActor>(ActorParams);

	// Root
	USceneComponent* Root = NewObject<USceneComponent>(PointerActor);
	Root->RegisterComponent();
	Root->SetWorldLocation(Position);
	PointerActor->SetRootComponent(Root);

	// Pointer
	UUxtFarPointerComponent* Pointer = NewObject<UUxtFarPointerComponent>(PointerActor);
	Pointer->RegisterComponent();
	Pointer->Hand = Hand;

	// Beam
	UUxtFarBeamComponent* Beam = NewObject<UUxtFarBeamComponent>(PointerActor);
	Beam->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	Beam->RegisterComponent();

	// Cursor
	UUxtFarCursorComponent* Cursor = NewObject<UUxtFarCursorComponent>(PointerActor);
	Cursor->AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
	Cursor->RegisterComponent();

	return Pointer;
}

UTestGrabTarget* UxtTestUtils::CreateNearPointerGrabTarget(
	UWorld* World, const FVector& Location, const FString& MeshFilename, float MeshScale)
{
	AActor* Actor = CreateNearPointerTargetActor(World, Location, MeshFilename, MeshScale);

	UTestGrabTarget* TestTarget = NewObject<UTestGrabTarget>(Actor);
	TestTarget->RegisterComponent();

	return TestTarget;
}

UTestPokeTarget* UxtTestUtils::CreateNearPointerPokeTarget(
	UWorld* World, const FVector& Location, const FString& MeshFilename, float MeshScale)
{
	AActor* Actor = CreateNearPointerTargetActor(World, Location, MeshFilename, MeshScale);

	UTestPokeTarget* TestTarget = NewObject<UTestPokeTarget>(Actor);
	TestTarget->RegisterComponent();

	return TestTarget;
}

UTestGrabTarget* UxtTestUtils::CreateNearPointerBackgroundTarget(UWorld* World)
{
	AActor* actor = World->SpawnActor<AActor>();

	UTestGrabTarget* testTarget = NewObject<UTestGrabTarget>(actor);
	testTarget->RegisterComponent();

	return testTarget;
}

USceneComponent* UxtTestUtils::CreateTestCamera(UWorld* World)
{
	AActor* CameraActor = World->SpawnActor<AActor>();
	USceneComponent* RootComponent = NewObject<USceneComponent>(CameraActor, TEXT("RootComponent"));
	CameraActor->SetRootComponent(RootComponent);
	UCameraComponent* CameraComponent = NewObject<UCameraComponent>(CameraActor, TEXT("Camera"));
	CameraComponent->SetupAttachment(RootComponent);

	APlayerController* OurPlayerController = UGameplayStatics::GetPlayerController(World, 0);
	OurPlayerController->SetViewTarget(CameraActor);

	return RootComponent;
}

bool FUxtDisableTestHandTrackerCommand::Update()
{
	UxtTestUtils::DisableTestHandTracker();
	return true;
}

void UxtTestUtils::SetTestHeadEnabled(bool bEnabled)
{
	UUxtFunctionLibrary::bUseTestData = bEnabled;
}

void UxtTestUtils::SetTestHeadLocation(const FVector& Location)
{
	UUxtFunctionLibrary::TestHeadPose.SetLocation(Location);
}

void UxtTestUtils::SetTestHeadRotation(const FRotator& Rotation)
{
	UUxtFunctionLibrary::TestHeadPose.SetRotation(Rotation.Quaternion());
}
