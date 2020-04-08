// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "CoreMinimal.h"
#include "Engine.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Tests/AutomationCommon.h"

#include "Behaviors/UxtFollowComponent.h"
#include "Utils/UxtFunctionLibrary.h"
#include "UxtTestUtils.h"

UUxtFollowComponent* CreateTestComponent(UWorld* World, const FVector& Location)
{
	AActor* actor = World->SpawnActor<AActor>();

	USceneComponent* root = NewObject<USceneComponent>(actor);
	actor->SetRootComponent(root);
	root->SetWorldLocation(Location);
	root->RegisterComponent();

	UUxtFollowComponent* testTarget = NewObject<UUxtFollowComponent>(actor);
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

	AActor* actorToFollow = World->SpawnActor<AActor>();
	USceneComponent* rootToFollow = NewObject<USceneComponent>(actorToFollow);
	actorToFollow->SetRootComponent(rootToFollow);
	rootToFollow->SetWorldLocation(Location + FVector::BackwardVector * testTarget->DefaultDistance);
	rootToFollow->RegisterComponent();

	testTarget->ActorToFollow = actorToFollow;

	return testTarget;
}

class FTestFollowComponentDistanceCommand : public IAutomationLatentCommand
{
public:
	FTestFollowComponentDistanceCommand(FAutomationTestBase* Test, UUxtFollowComponent* Follow)
		: Test(Test)
		, Follow(Follow)
		, bIgnoreDistance(Follow->bIgnoreDistanceClamp)
	{
	}

	virtual bool Update() override
	{
		// Two step update:
		// 
		// 1. Move the target past min distance extents 
		// 2. Check behaviour is expected for min bounds
		// 3. Move the target past max distance extents
		// 4. Check behaviour is expected for max bounds

		auto TargetTransform = Follow->ActorToFollow->GetTransform();
		auto FollowTransform = Follow->GetOwner()->GetTransform();

		auto TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		auto Distance = TargetToComponent.Size();
		TargetToComponent.Normalize();

		switch (UpdateCount)
		{
		case 0: // min distance
		{
			float MinFollowDist = Follow->MinimumDistance;

			auto ActorLocation = Follow->ActorToFollow->GetActorLocation();

			Follow->ActorToFollow->SetActorLocation(ActorLocation + TargetToComponent * (Distance - (MinFollowDist * 0.5f)));
			break;
		}
		case 1: // test follow still within bounds
		{
			Test->TestEqual("Follow component does not subceed minimum bounds", Distance < Follow->MinimumDistance, bIgnoreDistance);
			break;
		}
		case 2: // max distance
		{
			float MaxFollowDist = Follow->MaximumDistance;

			auto ActorLocation = Follow->ActorToFollow->GetActorLocation();

			Follow->ActorToFollow->SetActorLocation(ActorLocation - TargetToComponent * ((MaxFollowDist * 1.5f) - Distance));
			break;
		}
		case 3: // test follow still within bounds
		{
			Test->TestEqual("Follow component does not exceed maximum bounds", Distance > Follow->MaximumDistance, bIgnoreDistance);
			break;
		}
		}

		++UpdateCount;
		return (UpdateCount >= 4);
	}

private:

	FAutomationTestBase* Test;
	UUxtFollowComponent* Follow;

	bool bIgnoreDistance;

	int UpdateCount;
};

class FTestFollowComponentAngleCommand : public IAutomationLatentCommand
{
public:
	FTestFollowComponentAngleCommand(FAutomationTestBase* Test, UUxtFollowComponent* Follow)
		: Test(Test)
		, Follow(Follow)
		, bIgnoreAngular(Follow->bIgnoreAngleClamp)
	{
	}

	virtual bool Update() override
	{
		// Two step update:
		// 
		// 1. Move the target past horizontal angle extents 
		// 2. Check behaviour is expected for horizontal bounds
		// 3. Move the target past vertical angle extents
		// 4. Check behaviour is expected for vertical bounds

		auto TargetTransform = Follow->ActorToFollow->GetTransform();
		auto FollowTransform = Follow->GetOwner()->GetTransform();

		auto TargetToComponent = FollowTransform.GetLocation() - TargetTransform.GetLocation();
		auto TargetForward = TargetTransform.GetUnitAxis(EAxis::X);

		switch (UpdateCount)
		{
		case 0: // horizontal angle
		{
			float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, FVector::UpVector);
			float MaxHorizontal = FMath::DegreesToRadians(Follow->MaxViewHorizontalDegrees);

			auto NewTargetRot = FQuat(FVector::UpVector, (MaxHorizontal * 1.5f) - CurrAngle);

			Follow->ActorToFollow->SetActorRotation(NewTargetRot.Rotator());
			break;
		}
		case 1: // test follow still within bounds
		{
			float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, FVector::UpVector);

			Test->TestEqual("Follow component ignore angular option matches behaviour", FMath::IsNearlyZero(CurrAngle, 1.0e-7f), bIgnoreAngular);

			Test->TestTrue("Follow component does not exceed horizontal bounds", CurrAngle <= Follow->MaxViewHorizontalDegrees);
			break;
		}
		case 2: // vertical angle
		{
			float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, TargetTransform.GetUnitAxis(EAxis::Y));
			float MaxVertical = FMath::DegreesToRadians(Follow->MaxViewVerticalDegrees);

			auto NewTargetRot = FQuat(FVector::RightVector, (MaxVertical * 1.5f) - CurrAngle);

			Follow->ActorToFollow->SetActorRotation(NewTargetRot.Rotator());
			break;
		}
		case 3: // test follow still within bounds
		{
			float CurrAngle = AngleBetweenOnPlane(TargetForward, TargetToComponent, TargetTransform.GetUnitAxis(EAxis::Y));

			Test->TestEqual("Follow component ignore angular option matches behaviour", FMath::IsNearlyZero(CurrAngle, 1.0e-7f), bIgnoreAngular);

			Test->TestTrue("Follow component does not exceed vertical bounds", CurrAngle <= Follow->MaxViewVerticalDegrees);
			break;
		}
		}

		++UpdateCount;
		return (UpdateCount >= 4);
	}

private:

	float SimplifyAngle(float Angle)
	{
		while (Angle > PI)
		{
			Angle -= 2 * PI;
		}

		while (Angle < -PI)
		{
			Angle += 2 * PI;
		}

		return Angle;
	}

	float AngleBetweenOnPlane(FVector From, FVector To, FVector Normal)
	{
		From.Normalize();
		To.Normalize();
		Normal.Normalize();

		FVector Right = FVector::CrossProduct(Normal, From);
		FVector Forward = FVector::CrossProduct(Right, Normal);

		float Angle = FMath::Atan2(FVector::DotProduct(To, Right), FVector::DotProduct(To, Forward));

		return SimplifyAngle(Angle);
	}

	FAutomationTestBase* Test;
	UUxtFollowComponent* Follow;

	bool bIgnoreAngular;

	int UpdateCount;
};

class FTestFollowComponentOrientationCommand : public IAutomationLatentCommand
{
public:
	FTestFollowComponentOrientationCommand(FAutomationTestBase* Test, UUxtFollowComponent* Follow)
		: Test(Test)
		, Follow(Follow)
		, bFacing(Follow->OrientationType == EUxtFollowOrientBehavior::FaceCamera)
	{
		InitialRotation = Follow->GetOwner()->GetActorRotation().Quaternion();
	}

	virtual bool Update() override
	{
		// Two step update:
		// 
		// 1. Move the target halfway to the deadzone degrees
		// 2. Check behaviour is expected for orientation type
		// 3. Move the target past the deadzone degrees
		// 4. Check behaviour is expected for orientation type

		auto TargetTransform = Follow->ActorToFollow->GetTransform();
		auto FollowTransform = Follow->GetOwner()->GetTransform();

		switch (UpdateCount)
		{
		case 0: // move before deadzone
		{
			float DeadzoneAngle = FMath::DegreesToRadians(Follow->OrientToCameraDeadzoneDegrees);
			auto Rotation = FQuat(FVector::UpVector, DeadzoneAngle * 0.5f);

			auto ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();

			auto NewTargetPosition = FollowTransform.GetLocation() + Rotation * ComponentToTarget;

			Follow->ActorToFollow->SetActorLocation(NewTargetPosition);
			Follow->ActorToFollow->SetActorRotation(Rotation.Rotator());
			break;
		}
		case 1: // test orientation
		{
			auto ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();
			auto Cross = FVector::CrossProduct(FollowTransform.GetUnitAxis(EAxis::X), ComponentToTarget);

			Test->TestEqual("Follow component orientation has changed to match orientation type", InitialRotation != FollowTransform.GetRotation(), bFacing);

			Test->TestEqual("Follow component orientation type matches behaviour", FMath::IsNearlyZero(Cross.Size(), 0.001f), bFacing);
			break;
		}
		case 2: // move beyond deadzone
		{
			float DeadzoneAngle = FMath::DegreesToRadians(Follow->OrientToCameraDeadzoneDegrees);
			auto Rotation = FQuat(FVector::UpVector, DeadzoneAngle);

			auto ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();

			auto NewTargetPosition = FollowTransform.GetLocation() + Rotation * ComponentToTarget;

			Follow->ActorToFollow->SetActorLocation(NewTargetPosition);
			Follow->ActorToFollow->SetActorRotation(Rotation.Rotator());
			break;
		}
		case 3: // test orientation
		{
			auto ComponentToTarget = TargetTransform.GetLocation() - FollowTransform.GetLocation();
			auto Cross = FVector::CrossProduct(FollowTransform.GetUnitAxis(EAxis::X), ComponentToTarget);

			Test->TestNotEqual("Follow component orientation has changed", InitialRotation, FollowTransform.GetRotation());

			Test->TestTrue("Follow component orientation type matches behaviour", FMath::IsNearlyZero(Cross.Size(), 0.001f));
			break;
		}
		}

		++UpdateCount;
		return (UpdateCount >= 4);
	}

private:

	FAutomationTestBase* Test;
	UUxtFollowComponent* Follow;

	FQuat InitialRotation;

	bool bFacing;

	int UpdateCount;
};


IMPLEMENT_COMPLEX_AUTOMATION_TEST(FFollowComponentTest, "UXTools.FollowComponentTest",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ClientContext |
	EAutomationTestFlags::ProductFilter)

static const FString TestCase_IgnoreNothing = TEXT("TestCase_IgnoreNothing");
static const FString TestCase_IgnoreDistance = TEXT("TestCase_IgnoreDistance");
static const FString TestCase_IgnoreAngle = TEXT("TestCase_IgnoreAngle");

static const FString TestCase_Orientation_WorldLock = TEXT("TestCase_Orientation_WorldLock");
static const FString TestCase_Orientation_FaceCamera = TEXT("TestCase_Orientation_FaceUser");

void FFollowComponentTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	// Util for adding a test combination.
	auto AddTestCase = [&OutBeautifiedNames, &OutTestCommands](const FString& TestCase)
	{
		OutBeautifiedNames.Add(TestCase);
		OutTestCommands.Add(TestCase);
	};

	AddTestCase(TestCase_IgnoreNothing);
	AddTestCase(TestCase_IgnoreDistance);
	AddTestCase(TestCase_IgnoreAngle);

	AddTestCase(TestCase_Orientation_WorldLock);
	AddTestCase(TestCase_Orientation_FaceCamera);
}

bool FFollowComponentTest::RunTest(const FString& Parameters)
{
	FString TestCase = Parameters;

	// Load the empty test map to run the test in.
	AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty"));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForMapToLoadCommand());
	UWorld* World = UxtTestUtils::GetTestWorld();

	FVector Center(50, 0, 0);
	UUxtFollowComponent* Target = CreateTestComponent(World, Center);
	Target->MoveToDefaultDistanceLerpTime = 0;
	Target->bInterpolatePose = false;

	if (Parameters == TestCase_IgnoreNothing)
	{
		Target->bIgnoreDistanceClamp = false;
		Target->bIgnoreAngleClamp = false;
	}
	else if (Parameters == TestCase_IgnoreDistance)
	{
		Target->bIgnoreDistanceClamp = true;
		Target->bIgnoreAngleClamp = false;
	}
	else if (Parameters == TestCase_IgnoreAngle)
	{
		Target->bIgnoreDistanceClamp = false;
		Target->bIgnoreAngleClamp = true;
	}
	else if (Parameters == TestCase_Orientation_WorldLock)
	{
		Target->OrientationType = EUxtFollowOrientBehavior::WorldLock;
	}
	else if (Parameters == TestCase_Orientation_FaceCamera)
	{
		Target->OrientationType = EUxtFollowOrientBehavior::FaceCamera;
	}

	if (Parameters.Contains("Orientation"))
	{
		ADD_LATENT_AUTOMATION_COMMAND(FTestFollowComponentOrientationCommand(this, Target));
	}
	else
	{
		ADD_LATENT_AUTOMATION_COMMAND(FTestFollowComponentDistanceCommand(this, Target));

		ADD_LATENT_AUTOMATION_COMMAND(FTestFollowComponentAngleCommand(this, Target));
	}

	ADD_LATENT_AUTOMATION_COMMAND(FExitGameCommand());

	return true;
}

