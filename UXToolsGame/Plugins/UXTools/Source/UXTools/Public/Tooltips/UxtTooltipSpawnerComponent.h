// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interactions/UxtFarHandler.h"
#include "Interactions/UxtFarTarget.h"
#include "Interactions/UxtGrabHandler.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"

#include "UxtTooltipSpawnerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowTooltip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideTooltip);

/** The Vanish Type sets what action makes the tooltip vanish. */
UENUM(BlueprintType)
enum class EUxtTooltipVanishType : uint8
{
	/** The tooltip will vanish it loses focus. */
	VanishOnFocusExit = 0,
	/** The tooltip will vanish when it is being tapped/clicked on. */
	VanishOnTap,
};

/** Appear Type is to set the action that triggers the tooltip to be spawned. */
UENUM(BlueprintType)
enum class EUxtTooltipAppearType : uint8
{
	/** The tooltip will appear when the focus comes to this component. */
	AppearOnFocusEnter = 0,
	/** The tooltip will appear when this component is tapped/clicked. */
	AppearOnTap,
};

/** RemainType sets whether the tooltip uses the Lifetime to automatically destroy itself or if it stays on indefinitely. */
UENUM(BlueprintType)
enum class EUxtTooltipRemainType : uint8
{
	/** The tooltip will remain indefinitely. */
	Indefinite = 0,
	/** The tooltip will use the Lifetime value as its timeout. */
	Timeout,
};

/**
 *  Component used to "script" the behavior of a dynamic tooltip.
 *  This can be useful if your scene contains multiple tooltips and you don't want to create/render them all at once.
 *
 *  It's possible to configure upon what action the tooltip spawns/unspawn.
 *
 *  Different delays and lifetime can be configured.
 *
 *  OnHide and OnShow are broadcast when the tooltip is created/destroyed.
 *
 *  The Pivot can be used to offset the spawned tooltip.
 */
UCLASS(ClassGroup = ("UXTools - Experimental"), meta = (BlueprintSpawnableComponent))
class UXTOOLS_API UUxtTooltipSpawnerComponent
	: public USceneComponent
	, public IUxtGrabTarget
	, public IUxtGrabHandler
	, public IUxtFarTarget
	, public IUxtFarHandler
	, public IUxtPokeTarget
	, public IUxtPokeHandler
{
	GENERATED_UCLASS_BODY()

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Delegate to drive OnShow events. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tooltip Spawner - Experimental")
	FOnShowTooltip OnShowTooltip;

	/** Delegate to drive OnHide events. */
	UPROPERTY(BlueprintAssignable, Category = "Uxt Tooltip Spawner - Experimental")
	FOnHideTooltip OnHideTooltip;

protected:
	//
	// UActorComponent interface
	virtual void OnComponentCreated() override;

	//
	// IUxtGrabTarget interface
	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtGrabHandler interface
	virtual bool CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	///
	// IUxtFarTarget interface
	virtual bool IsFarFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtFarHandler interface
	virtual bool CanHandleFar_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnExitFarFocus_Implementation(UUxtFarPointerComponent* Pointer) override;
	virtual void OnFarPressed_Implementation(UUxtFarPointerComponent* Pointer) override;

	//
	// IUxtPokeTarget interface
	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtGrabHandler interface
	virtual bool CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

private:
	/** Triggers the tooltip creation procedure. */
	void CreateTooltip();

	/** Triggers the tooltip destruction procedure. */
	void DestroyTooltip();

	/** If we're in timeout mode, this function will schedule the destruction of the toolip. */
	void ScheduleDeathAfterLifetime();

	/** Parameter to script what will spawn the tooltip. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental")
	EUxtTooltipAppearType AppearType = EUxtTooltipAppearType::AppearOnFocusEnter;

	/** Parameter to script what will make the tooltip vanish. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental")
	EUxtTooltipVanishType VanishType = EUxtTooltipVanishType::VanishOnFocusExit;

	/** Parameter to script whether the tooltip remains indefinitely or if it uses the Lifetime parameter to timeout. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental")
	EUxtTooltipRemainType RemainType = EUxtTooltipRemainType::Timeout;

	/** Parameter to script a delay before spawning the tooltip. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", meta = (UIMin = "0.0", UIMax = "5.0"))
	float AppearDelay = 0.0f;

	/** Parameter to script a delay before the tooltip vanishes. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", meta = (UIMin = "0.0", UIMax = "5.0"))
	float VanishDelay = 2.0f;

	/** Parameter to script how long the tooltip will be spawned for. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", meta = (UIMin = "0.5", UIMax = "10.0"))
	float Lifetime = 1.0f;

	/** Parameter to script what widget class to use on this tooltip. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental")
	TSubclassOf<UUserWidget> WidgetClass = nullptr; // Pointer to the widget class it will instantiate.

	/** Parameter to script what text to use when no widget class has been set. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental")
	FText TooltipText;

	/** An offset to specify where the tooltip will be spawned. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", meta = (UseComponentPicker, AllowedClasses = "SceneComponent"))
	FComponentReference Pivot;

	/** Overrides the auto anchor on the spawned tooltip. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", AdvancedDisplay)
	bool bIsAutoAnchoring = true;

	/** Scales the widget. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", meta = (UIMin = "0.0", UIMax = "2.0"))
	FVector WidgetScale = FVector(1.0f, 1.0f, 1.0f);

	/** Margin adds a small margin around the text. */
	UPROPERTY(EditAnywhere, Category = "Uxt Tooltip Spawner - Experimental", meta = (UIMin = "0.0", UIMax = "2.0"))
	float Margin = 20.0f;

	/** Runtime variables */
	UPROPERTY(Transient)
	class AUxtTooltipActor* SpawnedTooltip = nullptr;

	/** Timers to control the Show/Hide. */
	FTimerHandle TimerHandle;
	/** Timers to control the Lifetime. */
	FTimerHandle LifetimeTimerHandle;

	/** Tightly coupled with these classes to keep the interface clean. */
	friend class FUxtTooltipSpawnerComponentVisualizer;
	friend class TooltipSpawnerSpec;
};
