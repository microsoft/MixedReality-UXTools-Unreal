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
UCLASS(ClassGroup = "UXToolsTests")
class UGrabTargetTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void OnEnterFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { EnterFarFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnUpdateFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { UpdateFarFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnExitFarFocus(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { ExitFarFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnEnterGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { EnterGrabFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnUpdateGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { UpdateGrabFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnExitGrabFocus(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { ExitGrabFocusCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnBeginGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer) { BeginGrabCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnUpdateGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer) { UpdateGrabCount++; }

	UFUNCTION(Category = "UXToolsTests")
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
