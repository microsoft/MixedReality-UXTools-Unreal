// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Tooltips/UxtTooltipActor.h"

#include "Components/SplineMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Controls/UxtBackPlateComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtFunctionLibrary.h"
#include "Widgets/Text/STextBlock.h"

AUxtTooltipActor::AUxtTooltipActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SceneRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRoot"));
	RootComponent = SceneRoot;
	SceneRoot->SetMobility(EComponentMobility::Movable);

	// Spline Mesh.
	SplineMeshComponent = ObjectInitializer.CreateDefaultSubobject<USplineMeshComponent>(this, TEXT("SplineMeshComponent"));
	SplineMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplineMeshComponent->Mobility = EComponentMobility::Movable;
	SplineMeshComponent->SetGenerateOverlapEvents(false);
	SplineMeshComponent->bAllowSplineEditingPerInstance = true;
	SplineMeshComponent->SetCastShadow(false);
	SplineMeshComponent->SetAbsolute(/*location*/ false, /*rotation*/ false, /*scale*/ true); // SplineMesh to not be affected by scale.

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("StaticMesh'/UXTools/Pointers/Meshes/SM_Tube.SM_Tube'"));
	check(Mesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/UXTools/Tooltip/M_TooltipSpline.M_TooltipSpline'"));
	check(Material.Object);

	SplineMeshComponent->SetStaticMesh(Mesh.Object);
	SplineMeshComponent->SetMaterial(0, Material.Object);
	SplineMeshComponent->SetHiddenInGame(true);
	SplineMeshComponent->SetupAttachment(SceneRoot);

	// Create the anchor component.
	Anchor = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, "Anchor");
	Anchor->SetupAttachment(SceneRoot);
	Anchor->SetAbsolute(/*location*/ false, /*rotation*/ false, /*scale*/ true); // Anchor to not be affected by scale.

	// Default value for the component target as most common.
	TooltipTarget.ComponentProperty = TEXT("SceneRoot");

	// Create and load the default widget so that there's something when instancing the tooltip.
	TooltipWidgetComponent = ObjectInitializer.CreateDefaultSubobject<UWidgetComponent>(this, "TooltipWidget");

	TooltipWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	TooltipWidgetComponent->SetDrawAtDesiredSize(true);
	TooltipWidgetComponent->SetTwoSided(true);
	TooltipWidgetComponent->SetupAttachment(SceneRoot);
	TooltipWidgetComponent->SetRelativeLocation(FVector(0, 0, 0), false);
	TooltipWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TooltipWidgetComponent->SetCastShadow(false);

	// Back Plate.
	BackPlate = ObjectInitializer.CreateDefaultSubobject<UUxtBackPlateComponent>(this, "BackPlate");
	BackPlate->SetupAttachment(SceneRoot);
	BackPlate->SetRelativeLocation(FVector(-0.01f, 0, 0), false);
	BackPlate->SetRelativeScale3D(FVector{0.f, 0.0f, 0.0f});
	BackPlate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BackPlate->SetCastShadow(false);
	BackPlate->SetAbsolute(/*location*/ false, /*rotation*/ false, /*scale*/ true); // BackPlate to not be affected by scale.

	// Set this actor to call Tick() every frame even in editor.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetActorTickEnabled(true);
}

void AUxtTooltipActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateComponent();
}

void AUxtTooltipActor::BeginPlay()
{
	Super::BeginPlay();

	USceneComponent* CurrentTooltipTarget = Cast<USceneComponent>(TooltipTarget.GetComponent(nullptr));
	if (CurrentTooltipTarget)
	{
		AttachToComponent(CurrentTooltipTarget, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void AUxtTooltipActor::SetTarget(AActor* TargetActor, UActorComponent* TargetComponent)
{
	TooltipTarget.OtherActor = TargetActor;
	TooltipTarget.OverrideComponent = TargetComponent;
	USceneComponent* CurrentTooltipTarget = Cast<USceneComponent>(TooltipTarget.GetComponent(nullptr));
	if (CurrentTooltipTarget)
	{
		AttachToComponent(CurrentTooltipTarget, FAttachmentTransformRules::KeepWorldTransform);
	}
}

void AUxtTooltipActor::SetText(const FText& Text)
{
	WidgetClass = nullptr;
	TooltipWidgetComponent->SetWidgetClass(nullptr);
	TooltipWidgetComponent->SetWidget(nullptr);
	TooltipWidgetComponent->SetSlateWidget(SNew(STextBlock).Text(Text));
}

FVector AUxtTooltipActor::GetClosestAnchorToTarget(FVector EndPosition) const
{
	FVector FinalStart = FVector::ZeroVector;
	float MaxSize = MAX_FLT;
	int MaxIndex = 0;
	const FVector WidgetScale = GetActorScale3D();
	const FVector2D WidgetSize = TooltipWidgetComponent->GetCurrentDrawSize() + Margin;

	static constexpr SSIZE_T NumAnchors = 8;
	const FVector AnchorsPosLocal[NumAnchors]{
		FVector(0.0f, 0.0f, WidgetSize.Y / 2.0f),                  // Top.
		FVector(0.0f, WidgetSize.X / 2.0f, WidgetSize.Y / 2.0f),   // Top-Right.
		FVector(0.0f, WidgetSize.X / 2.0f, 0.0f),                  // Right.
		FVector(0.0f, WidgetSize.X / 2.0f, -WidgetSize.Y / 2.0f),  // Bot-Right.
		FVector(0.0f, 0.0f, -WidgetSize.Y / 2.0f),                 // Bot.
		FVector(0.0f, -WidgetSize.X / 2.0f, -WidgetSize.Y / 2.0f), // Bot-Left.
		FVector(0.0f, -WidgetSize.X / 2.0f, 0.0f),                 // Left.
		FVector(0.0f, -WidgetSize.X / 2.0f, WidgetSize.Y / 2.0f),  // Top-Left.
	};

	// Apply scale and rotation to the anchor points and check which one is closest.
	for (SSIZE_T i = 0; i < NumAnchors; ++i)
	{
		FVector Start = AnchorsPosLocal[i] * WidgetScale;
		const FVector Delta = Start - EndPosition;
		float TempSize = Delta.Size();
		if (TempSize < MaxSize)
		{
			MaxIndex = i;
			MaxSize = TempSize;
			FinalStart = Start;
		}
	}
	return FinalStart;
}

void AUxtTooltipActor::UpdateBackPlate()
{
	// We won't get an accurate draw size if the widget hasn't been drawn once.
	// The render target seems to be a good enough indicator of that.
	if (!TooltipWidgetComponent->GetRenderTarget())
	{
		return;
	}

	const FVector ActorScale = GetActorScale3D();
	const FVector2D WidgetSize = TooltipWidgetComponent->GetCurrentDrawSize() + Margin;
	const FVector BackPlateScale = FVector(UUxtBackPlateComponent::GetDefaultBackPlateDepth(), WidgetSize.X, WidgetSize.Y) * ActorScale;
	BackPlate->SetRelativeScale3D(BackPlateScale);
}

// Function called often to update the state of the tooltip.
void AUxtTooltipActor::UpdateComponent()
{
	if (TooltipWidgetComponent == nullptr)
	{
		return;
	}
	UpdateWidget();

	UpdateBillboard();

	UpdateSpline();

	UpdateBackPlate();
}

void AUxtTooltipActor::UpdateSpline()
{
	if (const USceneComponent* CurrentTooltipTarget = Cast<USceneComponent>(TooltipTarget.GetComponent(nullptr)))
	{
		SplineMeshComponent->SetHiddenInGame(false);
		FVector StartWorldPos = TooltipWidgetComponent->GetComponentLocation();
		FVector EndWorldPos = CurrentTooltipTarget->GetComponentLocation();
		EndWorldPos += Anchor->GetRelativeLocation();

		// Billboarding rotates the actor so we need to compensate the rotation on the start/end points.
		const FTransform& InverseT = GetActorTransform().Inverse();
		EndWorldPos = InverseT.TransformPositionNoScale(EndWorldPos);
		StartWorldPos = InverseT.TransformPositionNoScale(StartWorldPos);

		FVector StartPos = FVector::ZeroVector;
		const FVector EndPos = EndWorldPos - StartWorldPos;
		if (bIsAutoAnchoring)
		{
			// The tooltip doesn't return the correct size until it has been rendered at least once. This coincides with the creation of the
			// render target.
			if (TooltipWidgetComponent->GetRenderTarget())
			{
				StartPos = GetClosestAnchorToTarget(EndPos);
				SplineMeshComponent->SetEndPosition(EndPos, false);
				SplineMeshComponent->SetStartPosition(StartPos, true);
			}
		}
		else
		{
			SplineMeshComponent->SetEndPosition(EndPos, false);
			SplineMeshComponent->SetStartPosition(StartPos, true);
		}
	}
	else
	{
		SplineMeshComponent->SetHiddenInGame(true);
	}
}

void AUxtTooltipActor::UpdateBillboard()
{
	// Billboard the widget.
	// Note:  UUxtFunctionLibrary::GetHeadPose returns invalid result outside of play mode (this code also runs in editor, so check begin
	// play)
	if (bIsBillboarding && HasActorBegunPlay())
	{
		FTransform HeadTransform = UUxtFunctionLibrary::GetHeadPose(GetWorld());
		const FVector TargetVector = HeadTransform.GetLocation() - TooltipWidgetComponent->GetComponentLocation();
		SetActorRotation(FRotationMatrix::MakeFromX(TargetVector).Rotator());
	}
}

void AUxtTooltipActor::UpdateWidget()
{
	const TSubclassOf<UUserWidget> CurrentWidgetClass = TooltipWidgetComponent->GetWidgetClass();
	const TSharedPtr<SWidget>& SlateWidget = TooltipWidgetComponent->GetSlateWidget();
	const UUserWidget* CurrentUserWidget = TooltipWidgetComponent->GetUserWidgetObject();

	if (SlateWidget == nullptr && WidgetClass == nullptr) // Got nothing then create default.
	{
		TooltipWidgetComponent->SetWidgetClass(nullptr);
		TooltipWidgetComponent->SetWidget(nullptr);
		TooltipWidgetComponent->SetSlateWidget(SNew(STextBlock).Text(FText::AsCultureInvariant("Default Widget")));
	}
	else if (WidgetClass && CurrentWidgetClass == nullptr) // Got a class configured by hasn't been set.
	{
		TooltipWidgetComponent->SetSlateWidget(nullptr);
		TooltipWidgetComponent->SetWidgetClass(WidgetClass);
	}
	else if (SlateWidget && WidgetClass) // We have a default but we also have a class configured.
	{
		TooltipWidgetComponent->SetSlateWidget(nullptr);
		TooltipWidgetComponent->SetWidgetClass(WidgetClass);
	}
	else if (CurrentWidgetClass != WidgetClass) // The widget class has changed.
	{
		TooltipWidgetComponent->SetWidgetClass(WidgetClass);
	}
	else if (CurrentUserWidget == nullptr && WidgetClass)
	{
		UUserWidget* CurrentWidget = CreateWidget(GetWorld(), WidgetClass);
		TooltipWidgetComponent->SetWidget(CurrentWidget);
	}
}

#if WITH_EDITORONLY_DATA
void AUxtTooltipActor::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	UpdateComponent();
	Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void AUxtTooltipActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdateComponent();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITORONLY_DATA

void AUxtTooltipActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateComponent();
}

bool AUxtTooltipActor::ShouldTickIfViewportsOnly() const
{
	return true;
}
