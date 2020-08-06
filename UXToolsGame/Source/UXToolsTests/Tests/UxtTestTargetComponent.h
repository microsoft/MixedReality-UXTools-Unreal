// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Misc/AutomationTest.h"

#include "FrameQueue.h"
#include "Interactions/UxtGrabTarget.h"
#include "Interactions/UxtPokeTarget.h"

#include "UxtTestTargetComponent.generated.h"

class UUxtNearPointerComponent;
class FUxtTestHandTracker;

/**
 * Target for grab tests that counts grab events.
 */
UCLASS()
class UXTOOLSTESTS_API UTestGrabTarget : public UActorComponent, public IUxtGrabTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface

	virtual void OnEnterGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnUpdateGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;
	virtual void OnExitGrabFocus_Implementation(UUxtNearPointerComponent* Pointer) override;

	virtual bool IsGrabFocusable_Implementation(const UPrimitiveComponent* Primitive) override;
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
UCLASS()
class UXTOOLSTESTS_API UTestPokeTarget : public UActorComponent, public IUxtPokeTarget
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

	//
	// IUxtGrabTarget interface

	virtual bool IsPokeFocusable_Implementation(const UPrimitiveComponent* Primitive) const override;
	virtual EUxtPokeBehaviour GetPokeBehaviour_Implementation() const override;
	virtual bool GetClosestPoint_Implementation(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const override;

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
