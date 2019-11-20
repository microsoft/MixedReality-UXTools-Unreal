#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine.h"
#include "EngineUtils.h"

// Copy of the hidden method GetAnyGameWorld() in AutomationCommon.cpp.
// Marked as temporary there, hence, this one is temporary, too.
UWorld* GetTestWorld() {
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts) {
		if (((Context.WorldType == EWorldType::PIE) || (Context.WorldType == EWorldType::Game))
			&& (Context.World() != nullptr)) {
			return Context.World();
		}
	}

	return nullptr;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlaceholderTest, "MixedRealityUtils.Placeholder",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

bool FPlaceholderTest::RunTest(const FString& Parameters)
{
	//AutomationOpenMap(TEXT("/Game/Levels/StartupLevel"));

	//UWorld* world = GetTestWorld();

	//TestTrue("GameMode class is set correctly",
	//	world->GetAuthGameMode()->IsA<YourGameModeBase>());
	//TestTrue("Essential actor is spawned",
	//	TActorIterator<AMyEssentialActor>(world));

	return true;
}
