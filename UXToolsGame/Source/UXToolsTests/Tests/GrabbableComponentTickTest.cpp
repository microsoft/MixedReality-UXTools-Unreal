#include "GrabbableComponentTickTest.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

#include <vector>

#include "UXToolsTestUtils.h"
#include "Input/UxtTouchPointer.h"

static UGrabbableTickTestComponent* CreateTestComponent(UWorld* World, const FVector& Location)
{
	AActor* actor = World->SpawnActor<AActor>();

	USceneComponent* root = NewObject<USceneComponent>(actor);
	actor->SetRootComponent(root);
	root->SetWorldLocation(Location);
	root->RegisterComponent();

	UGrabbableTickTestComponent* testTarget = NewObject<UGrabbableTickTestComponent>(actor);
	testTarget->RegisterComponent();

	FString meshFilename = TEXT("/Engine/BasicShapes/Cube.Cube");
	float meshScale = 0.3f;
	if (!meshFilename.IsEmpty())
	{
		UStaticMeshComponent* mesh = NewObject<UStaticMeshComponent>(actor);
		mesh->SetupAttachment(actor->GetRootComponent());
		mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		mesh->SetCollisionProfileName(TEXT("OverlapAll"));
		mesh->SetGenerateOverlapEvents(true);

		UStaticMesh* meshAsset = LoadObject<UStaticMesh>(actor, *meshFilename);
		mesh->SetStaticMesh(meshAsset);
		mesh->SetRelativeScale3D(FVector::OneVector * meshScale);

		mesh->RegisterComponent();
	}

	return testTarget;
}


class FSetPointerGraspCommand : public IAutomationLatentCommand
{
public:
	FSetPointerGraspCommand(UUxtTouchPointer* Pointer, bool bEnableGrasp)
		: Pointer(Pointer)
		, bEnableGrasp(bEnableGrasp)
	{}

	virtual bool Update() override
	{
		Pointer->SetGrasped(bEnableGrasp);
		return true;
	}

private:

	UUxtTouchPointer* Pointer;
	bool bEnableGrasp;
};

class FTestGrabbableComponentTickingCommand : public IAutomationLatentCommand
{
public:
	FTestGrabbableComponentTickingCommand(FAutomationTestBase* Test, UGrabbableTickTestComponent* Target, bool bExpectTicking)
		: Test(Test)
		, Target(Target)
		, bExpectTicking(bExpectTicking)
	{}

	virtual bool Update() override
	{
		bool bWasTicked = Target->GetNumTicks() > 0;
		Test->TestEqual(TEXT("Grabbable component ticked"), bExpectTicking, bWasTicked);
		Target->Reset();
		return true;
	}

private:

	FAutomationTestBase* Test;
	UGrabbableTickTestComponent* Target;
	bool bExpectTicking;
};


IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGrabbableComponentTickTest, "UXTools.GrabbableComponentTick",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

static const FString TestCase_ComponentTickDisabled = TEXT("ComponentTickDisabled");
static const FString TestCase_TickAlways = TEXT("TickAlways");
static const FString TestCase_TickOnlyWhileGrabbed = TEXT("TickOnlyWhileGrabbed");

void FGrabbableComponentTickTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(TestCase_ComponentTickDisabled);
	OutBeautifiedNames.Add(TestCase_TickAlways);
	OutBeautifiedNames.Add(TestCase_TickOnlyWhileGrabbed);

	OutTestCommands.Add(TestCase_ComponentTickDisabled);
	OutTestCommands.Add(TestCase_TickAlways);
	OutTestCommands.Add(TestCase_TickOnlyWhileGrabbed);
}

bool FGrabbableComponentTickTest::RunTest(const FString& Parameters)
{
	// Load the empty test map to run the test in.
	AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	UWorld *World = UXToolsTestUtils::GetTestWorld();

	FVector Center(150, 0, 0);
	UUxtTouchPointer* Pointer = UXToolsTestUtils::CreateTouchPointer(World, Center + FVector(-15, 0, 0));
	Pointer->SetTouchRadius(30);
	UGrabbableTickTestComponent* Target = CreateTestComponent(World, Center);

	bool bExpectUngraspedTicks = false;
	bool bExpectGraspedTicks = false;
	if (Parameters == TestCase_ComponentTickDisabled)
	{
		Target->SetTickOnlyWhileGrabbed(false);
		Target->SetComponentTickEnabled(false);
		bExpectUngraspedTicks = false;
		bExpectGraspedTicks = false;
	}
	if (Parameters == TestCase_TickAlways)
	{
		Target->SetTickOnlyWhileGrabbed(false);
		Target->SetComponentTickEnabled(true);
		bExpectUngraspedTicks = true;
		bExpectGraspedTicks = true;
	}
	if (Parameters == TestCase_TickOnlyWhileGrabbed)
	{
		Target->SetTickOnlyWhileGrabbed(true);
		bExpectUngraspedTicks = false;
		bExpectGraspedTicks = true;
	}

	// Register all new components.
	World->UpdateWorldComponents(false, false);

	// Have to update overlaps explicitly because the pointer doesn't move.
	Pointer->GetOwner()->UpdateOverlaps();

	ADD_LATENT_AUTOMATION_COMMAND(FSetPointerGraspCommand(Pointer, false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.2f));
	ADD_LATENT_AUTOMATION_COMMAND(FTestGrabbableComponentTickingCommand(this, Target, bExpectUngraspedTicks));

	ADD_LATENT_AUTOMATION_COMMAND(FSetPointerGraspCommand(Pointer, true));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.2f));
	ADD_LATENT_AUTOMATION_COMMAND(FTestGrabbableComponentTickingCommand(this, Target, bExpectGraspedTicks));

	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}
