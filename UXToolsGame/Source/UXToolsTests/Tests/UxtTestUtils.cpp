#include "UxtTestUtils.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include "Input/UxtTouchPointer.h"
#include "PointerTestSequence.h"

// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
// Marked as temporary there, hence, this one is temporary, too.
UWorld* UxtTestUtils::GetTestWorld() {
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts) {
		if (((Context.WorldType == EWorldType::PIE) || (Context.WorldType == EWorldType::Game))
			&& (Context.World() != nullptr)) {
			return Context.World();
		}
	}

	return nullptr;
}

UWorld* UxtTestUtils::CreateTestWorld()
{
	UWorld *world = UWorld::CreateWorld(EWorldType::PIE, true, TEXT("TestWorld"));
	return world;
}

UUxtTouchPointer* UxtTestUtils::CreateTouchPointer(UWorld *World, const FVector &Location, bool IsGrasped, bool AddMeshVisualizer)
{
	AActor *hand = World->SpawnActor<AActor>();

	USceneComponent *root = NewObject<USceneComponent>(hand);
	hand->SetRootComponent(root);
	root->SetWorldLocation(Location);
	root->RegisterComponent();

	UUxtTouchPointer *pointer = NewObject<UUxtTouchPointer>(hand);
	pointer->SetupAttachment(hand->GetRootComponent());
	pointer->SetGrasped(IsGrasped);
	pointer->SetTouchRadius(0.1f);
	pointer->RegisterComponent();

	if (AddMeshVisualizer)
	{
		UStaticMeshComponent *mesh = NewObject<UStaticMeshComponent>(hand);
		mesh->SetupAttachment(hand->GetRootComponent());
		mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		UStaticMesh *meshAsset = LoadObject<UStaticMesh>(hand, TEXT("/Engine/BasicShapes/Cube.Cube"));
		mesh->SetStaticMesh(meshAsset);
		mesh->SetRelativeScale3D(FVector::OneVector * 0.01f);

		mesh->RegisterComponent();
	}

	return pointer;
}

UTestTouchPointerTarget* UxtTestUtils::CreateTouchPointerTarget(UWorld *World, const FVector &Location, const FString &meshFilename, float meshScale)
{
	AActor *actor = World->SpawnActor<AActor>();

	USceneComponent *root = NewObject<USceneComponent>(actor);
	actor->SetRootComponent(root);
	root->SetWorldLocation(Location);
	root->RegisterComponent();

	UTestTouchPointerTarget *testTarget = NewObject<UTestTouchPointerTarget>(actor);
	testTarget->RegisterComponent();

	if (!meshFilename.IsEmpty())
	{
		UStaticMeshComponent *mesh = NewObject<UStaticMeshComponent>(actor);
		mesh->SetupAttachment(actor->GetRootComponent());
		mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		mesh->SetCollisionProfileName(TEXT("OverlapAll"));
		mesh->SetGenerateOverlapEvents(true);

		UStaticMesh *meshAsset = LoadObject<UStaticMesh>(actor, *meshFilename);
		mesh->SetStaticMesh(meshAsset);
		mesh->SetRelativeScale3D(FVector::OneVector * meshScale);

		mesh->RegisterComponent();
	}

	return testTarget;
}

UTestTouchPointerTarget* UxtTestUtils::CreateTouchPointerBackgroundTarget(UWorld* World)
{
	AActor* actor = World->SpawnActor<AActor>();

	UTestTouchPointerTarget* testTarget = NewObject<UTestTouchPointerTarget>(actor);
	testTarget->RegisterComponent();

	return testTarget;
}
