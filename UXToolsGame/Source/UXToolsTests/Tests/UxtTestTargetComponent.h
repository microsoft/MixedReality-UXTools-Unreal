// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "FrameQueue.h"

#include "Components/SceneComponent.h"
#include "Interactions/UxtGrabHandler.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeHandler.h"
#include "Interactions/UxtPokeTarget.h"
#include "Misc/AutomationTest.h"

#include "UxtTestTargetComponent.generated.h"

class UUxtNearPointerComponent;
class FUxtTestHandTracker;

/**
 * Target for grab tests that counts grab events.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UXTOOLSTESTS_API UTestGrabTarget
	: public UActorComponent
	, public IUxtGrabTarget
	, public IUxtGrabHandler
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface
	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;

	//
	// IUxtGrabHandler interface
	virtual bool CanHandleGrab_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnBeginGrab_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndGrab_Implementation(UUxtNearPointerComponent* Pointer) override;

	int BeginFocusCount;
	int EndFocusCount;

	int BeginGrabCount;
	int EndGrabCount;

	// If the target should enable focus lock on the pointer while grabbed.
	bool bUseFocusLock = false;
};

/**
 * Target for poke tests that counts poke events.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UXTOOLSTESTS_API UTestPokeTarget
	: public UActorComponent
	, public IUxtPokeTarget
	, public IUxtPokeHandler
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	//
	// IUxtPokeTarget interface
	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;
	virtual EUxtPokeBehaviour GetPokeBehaviour_Implementation() const override;
	virtual bool GetClosestPoint_Implementation(
		const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const override;

	//
	// IUxtPokeHandler interface
	virtual bool CanHandlePoke_Implementation(UPrimitiveComponent* Primitive) const override;
	virtual void OnEnterPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdatePokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitPokeFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual void OnBeginPoke_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnEndPoke_Implementation(UUxtNearPointerComponent* Pointer) override;

	int BeginFocusCount;
	int EndFocusCount;

	int BeginPokeCount;
	int EndPokeCount;

	// If the target should enable focus lock on the pointer while poked.
	bool bUseFocusLock = false;
};
