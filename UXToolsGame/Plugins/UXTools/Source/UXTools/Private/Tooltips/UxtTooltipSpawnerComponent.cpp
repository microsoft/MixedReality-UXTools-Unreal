// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Tooltips/UxtTooltipSpawnerComponent.h"

#include "TimerManager.h"

#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tooltips/UxtTooltipActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/UxtFunctionLibrary.h"

UUxtTooltipSpawnerComponent::UUxtTooltipSpawnerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetMobility(EComponentMobility::Movable);
}

void UUxtTooltipSpawnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(TimerHandle);
		TimerManager.ClearTimer(LifetimeTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUxtTooltipSpawnerComponent::OnComponentCreated()
{
	Super::OnComponentCreated();

	// This is to correct an offset that is automatically created when instancing the tooltip.
	SetRelativeLocation(FVector(0, 0, 0), false);
}

void UUxtTooltipSpawnerComponent::CreateTooltip()
{
	// Timer is used to schedule the "Appear delay".
	FTimerDelegate TimerCallback;
	TimerCallback.BindLambda([this] {
		SpawnedTooltip = GetWorld()->SpawnActor<AUxtTooltipActor>();
		SpawnedTooltip->TooltipTarget.OtherActor = GetOwner();
		SpawnedTooltip->TooltipTarget.OverrideComponent = GetOwner()->GetRootComponent();

		if (WidgetClass != nullptr)
		{
			SpawnedTooltip->WidgetClass = WidgetClass;
		}
		else if (!TooltipText.IsEmpty())
		{
			SpawnedTooltip->SetText(TooltipText);
		}
		SpawnedTooltip->bIsAutoAnchoring = bIsAutoAnchoring;

		AActor* Owner = GetOwner();
		const USceneComponent* PivotComponent = Cast<USceneComponent>(Pivot.GetComponent(Owner));
		const FVector Offset = PivotComponent ? PivotComponent->GetRelativeLocation() * Owner->GetActorScale3D() : FVector::ZeroVector;
		const FVector FinalTooltipLocation = GetOwner()->GetActorLocation() + Offset;
		SpawnedTooltip->SetActorLocation(FinalTooltipLocation);
		SpawnedTooltip->SetActorScale3D(WidgetScale);
		SpawnedTooltip->Margin = Margin;
		SpawnedTooltip->UpdateComponent();

		OnShowTooltip.Broadcast();
	});
	float FinalDelay = FMath::Max(AppearDelay, SMALL_NUMBER); // Timer handle needs time to be non-zero.
	GetOwner()->GetWorldTimerManager().SetTimer(TimerHandle, TimerCallback, FinalDelay, false, FinalDelay);

	if (RemainType == EUxtTooltipRemainType::Timeout)
	{
		ScheduleDeathAfterLifetime();
	}
}

void UUxtTooltipSpawnerComponent::DestroyTooltip()
{
	// Use timer to perform the "VanishDelay".
	FTimerDelegate TimerCallback;
	TimerCallback.BindLambda([this] {
		if (SpawnedTooltip)
		{
			GetWorld()->DestroyActor(SpawnedTooltip);
			SpawnedTooltip = nullptr;

			OnHideTooltip.Broadcast();
		}
	});
	auto FinalDelay = FMath::Max(VanishDelay, SMALL_NUMBER); // Timer handle needs time to be non-zero
	GetOwner()->GetWorldTimerManager().SetTimer(TimerHandle, TimerCallback, FinalDelay, false, FinalDelay);
}

void UUxtTooltipSpawnerComponent::ScheduleDeathAfterLifetime()
{
	// Timer is used to schedule the death of the tooltip based on Lifetime.
	FTimerDelegate LifetimeTimerCallback;
	LifetimeTimerCallback.BindLambda([this] {
		if (SpawnedTooltip)
		{
			GetWorld()->DestroyActor(SpawnedTooltip);
			SpawnedTooltip = nullptr;
			OnHideTooltip.Broadcast();
		}
	});
	float FinalLifetime = FMath::Max(Lifetime, SMALL_NUMBER); // Timer handle needs time to be non-zero
	GetOwner()->GetWorldTimerManager().SetTimer(LifetimeTimerHandle, LifetimeTimerCallback, FinalLifetime, false, FinalLifetime);
}

void UUxtTooltipSpawnerComponent::OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (SpawnedTooltip && VanishType == EUxtTooltipVanishType::VanishOnFocusExit)
	{
		DestroyTooltip();
	}
}

void UUxtTooltipSpawnerComponent::OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (SpawnedTooltip == nullptr && AppearType == EUxtTooltipAppearType::AppearOnTap)
	{
		CreateTooltip();
	}
	else if (SpawnedTooltip && VanishType == EUxtTooltipVanishType::VanishOnTap)
	{
		DestroyTooltip();
	}
}

bool UUxtTooltipSpawnerComponent::IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return true;
}

bool UUxtTooltipSpawnerComponent::CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UUxtTooltipSpawnerComponent::OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (SpawnedTooltip == nullptr && AppearType == EUxtTooltipAppearType::AppearOnFocusEnter)
	{
		CreateTooltip();
	}
}

void UUxtTooltipSpawnerComponent::OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (SpawnedTooltip && VanishType == EUxtTooltipVanishType::VanishOnFocusExit)
	{
		DestroyTooltip();
	}
}

bool UUxtTooltipSpawnerComponent::IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return true;
}

bool UUxtTooltipSpawnerComponent::CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UUxtTooltipSpawnerComponent::OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (SpawnedTooltip == nullptr && AppearType == EUxtTooltipAppearType::AppearOnFocusEnter)
	{
		CreateTooltip();
	}
}

void UUxtTooltipSpawnerComponent::OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer)
{
	if (SpawnedTooltip && VanishType == EUxtTooltipVanishType::VanishOnFocusExit)
	{
		DestroyTooltip();
	}
}

bool UUxtTooltipSpawnerComponent::IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const
{
	return true;
}

bool UUxtTooltipSpawnerComponent::CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const
{
	return true;
}

void UUxtTooltipSpawnerComponent::OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer)
{
	if (SpawnedTooltip == nullptr && AppearType == EUxtTooltipAppearType::AppearOnFocusEnter)
	{
		CreateTooltip();
	}
}
