// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Interactions/UxtGrabTargetComponent.h"

#include "GrabTargetTestComponent.generated.h"

class UUxtFarPointerComponent;
class UUxtNearPointerComponent;

/**
 * Target for UxtGrabTargetComponent tests that counts events.
 */
UCLASS()
class UGrabTargetTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnEnterFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { EnterFarFocusCount++; }

	UFUNCTION()
	void OnUpdateFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { UpdateFarFocusCount++; }

	UFUNCTION()
	void OnExitFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { ExitFarFocusCount++; }

	UFUNCTION()
	void OnEnterGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { EnterGrabFocusCount++; }

	UFUNCTION()
	void OnUpdateGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { UpdateGrabFocusCount++; }

	UFUNCTION()
	void OnExitGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { ExitGrabFocusCount++; }

	UFUNCTION()
	void OnBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer) { BeginGrabCount++; }

	UFUNCTION()
	void OnUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer) { UpdateGrabCount++; }

	UFUNCTION()
	void OnEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer) { EndGrabCount++; }

	int EnterFarFocusCount = 0;
	int UpdateFarFocusCount = 0;
	int ExitFarFocusCount = 0;

	int EnterGrabFocusCount = 0;
	int UpdateGrabFocusCount = 0;
	int ExitGrabFocusCount = 0;

	int BeginGrabCount = 0;
	int UpdateGrabCount = 0;
	int EndGrabCount = 0;
};
