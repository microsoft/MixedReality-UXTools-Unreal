// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "BoundsControlTestComponent.h"
#include "Engine.h"
#include "FrameQueue.h"
#include "UxtTestHand.h"
#include "UxtTestHandTracker.h"
#include "UxtTestUtils.h"

#include "Controls/UxtBoundsControlComponent.h"
#include "Input/UxtFarPointerComponent.h"
#include "Input/UxtNearPointerComponent.h"
#include "Interactions/Constraints/UxtMoveAxisConstraint.h"
#include "Interactions/Constraints/UxtRotationAxisConstraint.h"
#include "Interactions/UxtGenericManipulatorComponent.h"
#include "Tests/AutomationCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector TargetLocation(150, 0, 0);
	const FVector InitialPointerOffset(0, 200, 0); // Offset to avoid pointing at the target before test starts

	UUxtBoundsControlComponent* CreateTestComponent()
	{
		UWorld* World = UxtTestUtils::GetTestWorld();
		AActor* Actor = World->SpawnActor<AActor>();

		// Use the cone to test the box collider
		const FString MeshAssetRefName = TEXT("/Engine/BasicShapes/Cone.Cone");

		UStaticMeshComponent* Mesh = UxtTestUtils::CreateStaticMesh(Actor, FVector::OneVector, MeshAssetRefName);
		Actor->SetRootComponent(Mesh);
		Mesh->RegisterComponent();

		UUxtGenericManipulatorComponent* Manipulator = NewObject<UUxtGenericManipulatorComponent>(Actor);
		Manipulator->LerpTime = 0.0f;
		Manipulator->RegisterComponent();

		const FString BoundsControlPresetName = TEXT("/UXTools/BoundsControl/Presets/BoundsControlDefaultWithFaces");

		UUxtBoundsControlComponent* BoundsControl = NewObject<UUxtBoundsControlComponent>(Actor);
		BoundsControl->Config =
			Cast<UUxtBoundsControlConfig>(StaticLoadObject(UUxtBoundsControlConfig::StaticClass(), NULL, *BoundsControlPresetName));
		BoundsControl->RegisterComponent();

		Actor->SetActorLocation(TargetLocation);

		return BoundsControl;
	}

	// WARNING: This function becomes unsafe if the affordance map is modified during runtime at any point! In such case, consider
	// allocating the map elements in the heap (and maybe moving this functionality into BoundsControl itself).
	const FUxtAffordanceInstance* GetAffordanceInstance(UUxtBoundsControlComponent* BoundsControl, UPrimitiveComponent* Primitive)
	{
		check(BoundsControl);
		return BoundsControl->GetPrimitiveAffordanceMap().Find(Primitive);
	}

	UPrimitiveComponent* GetFocusedPrimitive(FUxtTestHand Hand)
	{
		UUxtPointerComponent* Pointer = Hand.GetPointer();
		if (auto* FarPointer = Cast<UUxtFarPointerComponent>(Pointer))
		{
			return FarPointer->GetHitPrimitive();
		}
		else if (auto* NearPointer = Cast<UUxtNearPointerComponent>(Pointer))
		{
			FVector ClosestPoint, Normal;
			return NearPointer->GetFocusedGrabPrimitive(ClosestPoint, Normal);
		}
		return nullptr;
	}
} // namespace

BEGIN_DEFINE_SPEC(
	BoundsControlSpec, "UXTools.BoundsControl", EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask)

void SetupEventCaptureComponent(UUxtGrabTargetComponent* GrabTarget);
bool TestQuatEqual(const FString& What, const FQuat& Actual, const FQuat& Expected, float Tolerance = KINDA_SMALL_NUMBER);
void EnqueueNoFocusedAffordanceChecks();
void EnqueueAffordanceScaledWithDistanceTest();
void EnqueueTransformationsTest();
void EnqueueAffordanceFocusTest();
void EnqueueRotationAroundAxisChecks(const FVector& AllowedAxis);
void EnqueueTranslationAlongAxisChecks(const FVector& AllowedAxis);

FFrameQueue FrameQueue;

AActor* Actor;

FUxtTestHand LeftHand = FUxtTestHand(EControllerHand::Left);
FUxtTestHand RightHand = FUxtTestHand(EControllerHand::Right);

// Components
UUxtBoundsControlComponent* Target;
UPrimitiveComponent* TargetPrimitive;
UBoundsControlTestComponent* EventCaptureComponent;

// Data
FTransform InitialActorTransform;
FVector InitialAffordanceScale;
EUxtInteractionMode InteractionMode = EUxtInteractionMode::None; // Cached to avoid passing it through all EnqueueX/lambdas

// Constraints
UUxtRotationAxisConstraint* RotationConstraint;
UUxtMoveAxisConstraint* TranslationConstraint;

END_DEFINE_SPEC(BoundsControlSpec)

void BoundsControlSpec::Define()
{
	BeforeEach(
		[this]
		{
			TestTrueExpr(AutomationOpenMap(TEXT("/Game/UXToolsGame/Tests/Maps/TestEmpty")));

			UWorld* World = UxtTestUtils::GetTestWorld();
			FrameQueue.Init(&World->GetGameInstance()->GetTimerManager());

			UxtTestUtils::EnableTestInputSystem();

			Target = CreateTestComponent();

			Actor = Target->GetOwner();
			InitialActorTransform = Actor->GetActorTransform();

			SetupEventCaptureComponent(Cast<UUxtGrabTargetComponent>(
				Target->GetBoundsControlActor()->GetComponentByClass(UUxtGrabTargetComponent::StaticClass())));
			TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::CornerFrontRightTop);
		});

	AfterEach(
		[this]
		{
			FrameQueue.Reset();

			UxtTestUtils::DisableTestInputSystem();

			Actor->Destroy();
		});

	Describe(
		"Far interaction",
		[this]
		{
			BeforeEach(
				[this]
				{
					InteractionMode = EUxtInteractionMode::Far;
					LeftHand.Configure(InteractionMode, TargetPrimitive->GetComponentLocation() + InitialPointerOffset);
				});

			AfterEach([this] { LeftHand.Reset(); });

			EnqueueAffordanceScaledWithDistanceTest();
			EnqueueTransformationsTest();
			EnqueueAffordanceFocusTest();

			LatentIt(
				"should focus collision box",
				[this](const FDoneDelegate& Done)
				{
					EnqueueNoFocusedAffordanceChecks();
					FrameQueue.Enqueue(
						[this]
						{
							TestNull("Pointer is focusing nothing beforehand", LeftHand.GetPointer()->GetFocusTarget());
							const FBox& Bounds = Target->GetBounds();
							const float OffsetZ =
								Bounds.GetCenter().Z + (Bounds.GetExtent().Z / 2); // Avoid both top and center affordances
							const FVector TargetPoint = Bounds.GetCenter() + FVector(-Bounds.GetExtent().X, 0, OffsetZ);
							LeftHand.SetTranslation(TargetPoint, /* bApplyOffset = */ true);
						});
					EnqueueNoFocusedAffordanceChecks();
					FrameQueue.Enqueue(
						[this]
						{
							TestNotNull("Pointer is focusing something", LeftHand.GetPointer()->GetFocusTarget());
							UPrimitiveComponent* HitPrimitive = GetFocusedPrimitive(LeftHand);
							TestEqual(
								"Pointer is focusing the collision box", HitPrimitive, Cast<UPrimitiveComponent>(Target->CollisionBox));
						});
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});

	Describe(
		"Near interaction",
		[this]
		{
			BeforeEach(
				[this]
				{
					InteractionMode = EUxtInteractionMode::Near;
					LeftHand.SetLocationOffset(FVector(-1, 0, 0), EUxtInteractionMode::Near);
					LeftHand.Configure(InteractionMode, InitialPointerOffset);
				});

			AfterEach([this] { LeftHand.Reset(); });

			EnqueueAffordanceScaledWithDistanceTest();
			EnqueueTransformationsTest();
			EnqueueAffordanceFocusTest();

			LatentIt(
				"should update affordances when the parent actor is moved",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this] { LeftHand.Translate(TargetPrimitive->GetComponentLocation()); });
					FrameQueue.Enqueue([this] { LeftHand.SetGrabbing(true); });
					FrameQueue.Enqueue([this] { LeftHand.Translate(FVector::RightVector * 10); });
					FrameQueue.Enqueue(
						[this]()
						{
							const FTransform ActorTransform = Actor->GetTransform();
							const FBox& Bounds = Target->GetBounds();

							for (const auto& Entry : Target->GetPrimitiveAffordanceMap())
							{
								const UPrimitiveComponent* AffordancePrimitive = Entry.Key;
								const FUxtAffordanceInstance& AffordanceInstance = Entry.Value;

								FVector ExpectedLocation;
								FQuat ExpectedRotation;
								AffordanceInstance.Config.GetWorldLocationAndRotation(
									Bounds, ActorTransform, ExpectedLocation, ExpectedRotation);

								TestEqual("Affordance location has updated", AffordancePrimitive->GetComponentLocation(), ExpectedLocation);
								TestQuatEqual("Affordance rotation has updated", AffordancePrimitive->GetComponentQuat(), ExpectedRotation);
							}
						});
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});
}

void BoundsControlSpec::SetupEventCaptureComponent(UUxtGrabTargetComponent* GrabTarget)
{
	EventCaptureComponent = NewObject<UBoundsControlTestComponent>(Target);

	GrabTarget->OnBeginGrab.AddDynamic(EventCaptureComponent, &UBoundsControlTestComponent::OnStartGrab);
	GrabTarget->OnEndGrab.AddDynamic(EventCaptureComponent, &UBoundsControlTestComponent::OnEndGrab);
	GrabTarget->OnEnterGrabFocus.AddDynamic(EventCaptureComponent, &UBoundsControlTestComponent::OnNearAffordanceFocusEnter);
	GrabTarget->OnExitGrabFocus.AddDynamic(EventCaptureComponent, &UBoundsControlTestComponent::OnNearAffordanceFocusExit);
	GrabTarget->OnEnterFarFocus.AddDynamic(EventCaptureComponent, &UBoundsControlTestComponent::OnFarAffordanceFocusEnter);
	GrabTarget->OnExitFarFocus.AddDynamic(EventCaptureComponent, &UBoundsControlTestComponent::OnFarAffordanceFocusExit);

	EventCaptureComponent->RegisterComponent();
}

bool BoundsControlSpec::TestQuatEqual(const FString& What, const FQuat& Actual, const FQuat& Expected, float Tolerance)
{
	return TestEqual(What, Actual.GetAxisX(), Expected.GetAxisX(), Tolerance) &&
		   TestEqual(What, Actual.GetAxisY(), Expected.GetAxisY(), Tolerance) &&
		   TestEqual(What, Actual.GetAxisZ(), Expected.GetAxisZ(), Tolerance);
}

void BoundsControlSpec::EnqueueNoFocusedAffordanceChecks()
{
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Near focus entered", EventCaptureComponent->NearFocusEnterCount, 0);
			TestEqual("Near focus exited", EventCaptureComponent->NearFocusExitCount, 0);
			TestEqual("Far focus entered", EventCaptureComponent->FarFocusEnterCount, 0);
			TestEqual("Far focus exited", EventCaptureComponent->FarFocusExitCount, 0);
		});
}

void BoundsControlSpec::EnqueueAffordanceScaledWithDistanceTest()
{
	LatentIt(
		"should scale affordance primitives based on distance to actor's center",
		[this](const FDoneDelegate& Done)
		{
			EnqueueNoFocusedAffordanceChecks();
			FrameQueue.Enqueue(
				[this]
				{
					InitialAffordanceScale = TargetPrimitive->GetRelativeScale3D();
					Target->AffordanceTransitionDuration = 1.0f; // Disable affordance animation, so scaling doesn't take multiple frames
					TestEqual("Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 0);
					LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
				});
			FrameQueue.Enqueue(
				[this]
				{
					TestEqual("Pointer focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
					TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
					LeftHand.SetGrabbing(true);
				});
			FrameQueue.Enqueue(
				[this]
				{
					TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
					LeftHand.Translate(FVector(0, 10, 0));
					LeftHand.SetGrabbing(false);
				});
			FrameQueue.Enqueue(
				[this]
				{
					TestEqual("Released affordance grab", EventCaptureComponent->GetGrabEndCount(InteractionMode), 1);
					LeftHand.SetTranslation(FVector::ZeroVector);
				});
			FrameQueue.Enqueue(
				[this]
				{
					// Make sure that the increased size is not due to being grabbed/focused
					TestEqual("Affordance received focus exit", EventCaptureComponent->GetFocusExitCount(InteractionMode), 1);
					TestNotEqual("Pointer isn't focusing affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);

					// This doesn't use the exact math because it's cumbersome and susceptible to change
					TestTrue(
						"Affordance was scaled after actor resize",
						TargetPrimitive->GetRelativeScale3D().Size() > InitialAffordanceScale.Size());
				});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
}

void BoundsControlSpec::EnqueueTransformationsTest()
{
	Describe(
		"without external constraints",
		[this]
		{
			LatentIt(
				"clamps to min scale",
				[this](const FDoneDelegate& Done)
				{
					EnqueueNoFocusedAffordanceChecks();
					FrameQueue.Enqueue(
						[this]
						{
							Target->SetRelativeToInitialScale(true);
							TestEqual(
								"Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode),
								0);
							LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
							TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
							LeftHand.SetGrabbing(true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
							LeftHand.Translate(FVector(0, -1000, 0));
						});
					FrameQueue.Enqueue(
						[this] {
							TestEqual(
								"Scale was clamped to min value", Actor->GetActorScale3D(),
								InitialActorTransform.GetScale3D() * Target->GetMinScale());
						});
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"clamps to max scale",
				[this](const FDoneDelegate& Done)
				{
					EnqueueNoFocusedAffordanceChecks();
					FrameQueue.Enqueue(
						[this]
						{
							Target->SetRelativeToInitialScale(true);
							TestEqual(
								"Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode),
								0);
							LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
							TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
							LeftHand.SetGrabbing(true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
							LeftHand.Translate(FVector(0, 1000, 0));
						});
					FrameQueue.Enqueue(
						[this] {
							TestEqual(
								"Scale was clamped to max value", Actor->GetActorScale3D(),
								InitialActorTransform.GetScale3D() * Target->GetMaxScale());
						});
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"prevents scaling with min/max scale = 1",
				[this](const FDoneDelegate& Done)
				{
					EnqueueNoFocusedAffordanceChecks();
					FrameQueue.Enqueue(
						[this]
						{
							Target->SetRelativeToInitialScale(true);
							Target->SetMinScale(1);
							Target->SetMaxScale(1);
							TestEqual(
								"Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode),
								0);
							LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
							TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
							LeftHand.SetGrabbing(true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
							LeftHand.Translate(FVector(0, -1000, 0));
						});
					FrameQueue.Enqueue([this]
									   { TestEqual("Doesn't scale down", Actor->GetActorScale3D(), InitialActorTransform.GetScale3D()); });
					FrameQueue.Enqueue([this] { LeftHand.Translate(FVector(0, 2000, 0)); });
					FrameQueue.Enqueue([this]
									   { TestEqual("Doesn't scale up", Actor->GetActorScale3D(), InitialActorTransform.GetScale3D()); });
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"doesn't rotate with corner affordance",
				[this](const FDoneDelegate& Done)
				{
					EnqueueNoFocusedAffordanceChecks();
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual(
								"Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode),
								0);
							LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
							TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
							LeftHand.SetGrabbing(true);
						});
					FrameQueue.Enqueue(
						[this]
						{
							TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
							LeftHand.Translate(FVector(0, -1000, 0));
						});
					FrameQueue.Enqueue(
						[this] {
							TestEqual("Actor rotation unchanged", Actor->GetActorRotation(), InitialActorTransform.GetRotation().Rotator());
						});
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"rotates around UpVector",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this]
									   { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeFrontRight); });
					EnqueueRotationAroundAxisChecks(FVector::UpVector);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"rotates around RightVector",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue([this] { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeFrontTop); });
					EnqueueRotationAroundAxisChecks(FVector::RightVector);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});

			LatentIt(
				"rotates around ForwardVector",
				[this](const FDoneDelegate& Done)
				{
					FrameQueue.Enqueue(
						[this]
						{
							// As this requires to target an affordance on the side face, rotate the actor and re-cache the initial transform
							Actor->SetActorRotation(FRotator(0, 90, 0));
							InitialActorTransform = Actor->GetActorTransform();
							TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeRightTop);
						});
					EnqueueRotationAroundAxisChecks(FVector::ForwardVector);
					FrameQueue.Enqueue([Done] { Done.Execute(); });
				});
		});

	Describe(
		"with rotation constraint",
		[this]
		{
			BeforeEach(
				[this]
				{
					RotationConstraint = NewObject<UUxtRotationAxisConstraint>(Actor);
					RotationConstraint->RegisterComponent();
				});

			Describe(
				"X allowed in local space",
				[this]
				{
					BeforeEach(
						[this]
						{
							RotationConstraint->AllowedAxis = EUxtAxis::X;
							RotationConstraint->bUseLocalSpaceForConstraint = true;
						});

					LatentIt(
						"doesn't rotate around UpVector",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this] { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeFrontRight); });
							EnqueueRotationAroundAxisChecks(FVector::ZeroVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"doesn't rotate around RightVector",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this] { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeFrontTop); });
							EnqueueRotationAroundAxisChecks(FVector::ZeroVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"rotates around ForwardVector",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this]
								{
									// As this requires to target an affordance on the side face, rotate the actor and re-cache the initial
									// transform
									Actor->SetActorRotation(FRotator(0, 90, 0));
									InitialActorTransform = Actor->GetActorTransform();
									TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeRightTop);
								});
							EnqueueRotationAroundAxisChecks(FVector::ForwardVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});
				});

			Describe(
				"Y allowed in world space",
				[this]
				{
					BeforeEach(
						[this]
						{
							RotationConstraint->AllowedAxis = EUxtAxis::Y;
							RotationConstraint->bUseLocalSpaceForConstraint = false;
						});

					LatentIt(
						"doesn't rotate around UpVector",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this] { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeFrontRight); });
							EnqueueRotationAroundAxisChecks(FVector::ZeroVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"rotates around RightVector",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this] { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeFrontTop); });
							EnqueueRotationAroundAxisChecks(FVector::RightVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"rotates around ForwardVector",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this]
								{
									// As this requires to target an affordance on the side face, rotate the actor and re-cache the initial
									// transform
									Actor->SetActorRotation(FRotator(0, 90, 0));
									InitialActorTransform = Actor->GetActorTransform();
									TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::EdgeRightTop);
								});
							// NOTE that the actor was rotated to target the side affordance more easily, so actor's X is aligned to world's Y
							EnqueueRotationAroundAxisChecks(FVector::ForwardVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});
				});
		});

	Describe(
		"with translation constraint",
		[this]
		{
			BeforeEach(
				[this]
				{
					TranslationConstraint = NewObject<UUxtMoveAxisConstraint>(Actor);
					TranslationConstraint->RegisterComponent();
				});

			Describe(
				"restricting local Y|Z",
				[this]
				{
					BeforeEach(
						[this]
						{
							TranslationConstraint->ConstraintOnMovement = static_cast<uint32>(EUxtAxisFlags::Y | EUxtAxisFlags::Z);
							TranslationConstraint->bUseLocalSpaceForConstraint = true;
						});

					LatentIt(
						"moves along local X",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue([this]
											   { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::FaceFront); });
							EnqueueTranslationAlongAxisChecks(FVector::ForwardVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"doesn't move along local Y",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this]
								{
									// As this requires to target the affordance on the top face, rotate the actor and re-cache the initial
									// transform
									Actor->SetActorRotation(FRotator(0, 90, 0));
									InitialActorTransform = Actor->GetActorTransform();
									TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::FaceRight);
								});
							EnqueueTranslationAlongAxisChecks(FVector::ZeroVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"doesn't move along local Z",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this]
								{
									// As this requires to target the affordance on the top face, rotate the actor and re-cache the initial
									// transform
									Actor->SetActorRotation(FRotator(90, 0, 0));
									InitialActorTransform = Actor->GetActorTransform();
									TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::FaceTop);
								});
							EnqueueTranslationAlongAxisChecks(FVector::ZeroVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});
				});

			// As world X is allowed and actor is rotated so the appropriate affordance is on the X axis, all must succeed
			Describe(
				"restricting world Y|Z",
				[this]
				{
					BeforeEach(
						[this]
						{
							TranslationConstraint->ConstraintOnMovement = static_cast<uint32>(EUxtAxisFlags::Y | EUxtAxisFlags::Z);
							TranslationConstraint->bUseLocalSpaceForConstraint = false;
						});

					LatentIt(
						"moves along local X",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue([this]
											   { TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::FaceFront); });
							EnqueueTranslationAlongAxisChecks(FVector::ForwardVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"moves along local Z",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this]
								{
									// As this requires to target the affordance on the top face, rotate the actor and re-cache the initial
									// transform
									Actor->SetActorRotation(FRotator(90, 0, 0));
									InitialActorTransform = Actor->GetActorTransform();
									TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::FaceTop);
								});
							EnqueueTranslationAlongAxisChecks(FVector::ForwardVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});

					LatentIt(
						"moves along local Y",
						[this](const FDoneDelegate& Done)
						{
							FrameQueue.Enqueue(
								[this]
								{
									// As this requires to target the affordance on the top face, rotate the actor and re-cache the initial
									// transform
									Actor->SetActorRotation(FRotator(0, 90, 0));
									InitialActorTransform = Actor->GetActorTransform();
									TargetPrimitive = Target->GetAffordancePrimitive(EUxtAffordancePlacement::FaceRight);
								});
							EnqueueTranslationAlongAxisChecks(FVector::ForwardVector);
							FrameQueue.Enqueue([Done] { Done.Execute(); });
						});
				});
		});
}

void BoundsControlSpec::EnqueueAffordanceFocusTest()
{
	LatentIt(
		"should raise enter/exit affordance focus",
		[this](const FDoneDelegate& Done)
		{
			EnqueueNoFocusedAffordanceChecks();
			FrameQueue.Enqueue([this] { LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), true); });
			FrameQueue.Enqueue(
				[this]
				{
					TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
					TestEqual("Affordance has 1 focusing pointer", GetAffordanceInstance(Target, TargetPrimitive)->FocusCount, 1);
					TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
					LeftHand.SetTranslation(InitialPointerOffset);
				});
			FrameQueue.Enqueue(
				[this]
				{
					TestNull("Pointer is focusing something", GetFocusedPrimitive(LeftHand));
					TestEqual("Affordance has no focusing pointers", GetAffordanceInstance(Target, TargetPrimitive)->FocusCount, 0);
					TestEqual("Affordance received focus exit", EventCaptureComponent->GetFocusExitCount(InteractionMode), 1);
				});
			FrameQueue.Enqueue([Done] { Done.Execute(); });
		});
}

void BoundsControlSpec::EnqueueRotationAroundAxisChecks(const FVector& AllowedAxes)
{
	EnqueueNoFocusedAffordanceChecks();
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 0);
			LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
		});
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
			TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
			LeftHand.SetGrabbing(true);
		});
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
			// Simpler than parameterising, since each affordance should only rotate around a single axis
			LeftHand.Translate(FVector(5, 5, 5));
		});
	FrameQueue.Enqueue(
		[this, AllowedAxes]
		{
			const FVector InitialRot = InitialActorTransform.GetRotation().Rotator().Euler();
			const FVector CurrentRot = Actor->GetActorRotation().Euler();
			const bool bIsNearlyEqualX = FMath::IsNearlyEqual(InitialRot.X, CurrentRot.X, 0.001f);
			const bool bIsNearlyEqualY = FMath::IsNearlyEqual(InitialRot.Y, CurrentRot.Y, 0.001f);
			const bool bIsNearlyEqualZ = FMath::IsNearlyEqual(InitialRot.Z, CurrentRot.Z, 0.001f);
			TestTrue("Correct X value", FMath::IsNearlyZero(AllowedAxes.X) ? bIsNearlyEqualX : !bIsNearlyEqualX);
			TestTrue("Correct Y value", FMath::IsNearlyZero(AllowedAxes.Y) ? bIsNearlyEqualY : !bIsNearlyEqualY);
			TestTrue("Correct Z value", FMath::IsNearlyZero(AllowedAxes.Z) ? bIsNearlyEqualZ : !bIsNearlyEqualZ);
		});
}

void BoundsControlSpec::EnqueueTranslationAlongAxisChecks(const FVector& AllowedAxes)
{
	EnqueueNoFocusedAffordanceChecks();
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Affordance didn't receive focus beforehand", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 0);
			LeftHand.SetTranslation(TargetPrimitive->GetComponentLocation(), /* bApplyOffset = */ true);
		});
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Pointer is focusing the right affordance", GetFocusedPrimitive(LeftHand), TargetPrimitive);
			TestEqual("Affordance received focus enter", EventCaptureComponent->GetFocusEnterCount(InteractionMode), 1);
			LeftHand.SetGrabbing(true);
		});
	FrameQueue.Enqueue(
		[this]
		{
			TestEqual("Started affordance grab", EventCaptureComponent->GetGrabStartCount(InteractionMode), 1);
			LeftHand.Translate(FVector(5, 0, 0));
		});
	FrameQueue.Enqueue(
		[this, AllowedAxes]
		{
			const FVector InitialLoc = InitialActorTransform.GetLocation();
			const FVector CurrentLoc = Actor->GetActorLocation();
			const bool bIsNearlyEqualX = FMath::IsNearlyEqual(InitialLoc.X, CurrentLoc.X, 0.001f);
			const bool bIsNearlyEqualY = FMath::IsNearlyEqual(InitialLoc.Y, CurrentLoc.Y, 0.001f);
			const bool bIsNearlyEqualZ = FMath::IsNearlyEqual(InitialLoc.Z, CurrentLoc.Z, 0.001f);
			TestTrue("Correct X value", FMath::IsNearlyZero(AllowedAxes.X) ? bIsNearlyEqualX : !bIsNearlyEqualX);
			TestTrue("Correct Y value", FMath::IsNearlyZero(AllowedAxes.Y) ? bIsNearlyEqualY : !bIsNearlyEqualY);
			TestTrue("Correct Z value", FMath::IsNearlyZero(AllowedAxes.Z) ? bIsNearlyEqualZ : !bIsNearlyEqualZ);
		});
}

#endif
