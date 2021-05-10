// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Interactions/UxtGrabTargetComponent.h"

#include "BoundsControlTestComponent.generated.h"

/**
 * Target for Bounds Control tests that counts total focus and grabs events.
 */
UCLASS(ClassGroup = "UXToolsTests")
class UBoundsControlTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "UXToolsTests")
	void OnStartGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
	{
		if (GrabPointer.NearPointer)
		{
			NearGrabStartCount++;
		}
		else
		{
			FarGrabStartCount++;
		}
	}

	UFUNCTION(Category = "UXToolsTests")
	void OnEndGrab(UUxtGrabTargetComponent* Grabbable, FUxtGrabPointerData GrabPointer)
	{
		if (GrabPointer.NearPointer)
		{
			NearGrabEndCount++;
		}
		else
		{
			FarGrabEndCount++;
		}
	}

	UFUNCTION(Category = "UXToolsTests")
	void OnNearAffordanceFocusEnter(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { NearFocusEnterCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnNearAffordanceFocusExit(UUxtGrabTargetComponent* Grabbable, UUxtNearPointerComponent* Pointer) { NearFocusExitCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnFarAffordanceFocusEnter(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { FarFocusEnterCount++; }

	UFUNCTION(Category = "UXToolsTests")
	void OnFarAffordanceFocusExit(UUxtGrabTargetComponent* Grabbable, UUxtFarPointerComponent* Pointer) { FarFocusExitCount++; }

	int GetGrabStartCount(EUxtInteractionMode Mode)
	{
		switch (Mode)
		{
		case EUxtInteractionMode::Near:
			return NearGrabStartCount;
		case EUxtInteractionMode::Far:
			return FarGrabStartCount;
		case EUxtInteractionMode::None:
			checkNoEntry();
			break;
		}
		return -1;
	}

	int GetGrabEndCount(EUxtInteractionMode Mode)
	{
		switch (Mode)
		{
		case EUxtInteractionMode::Near:
			return NearGrabEndCount;
		case EUxtInteractionMode::Far:
			return FarGrabEndCount;
		case EUxtInteractionMode::None:
			checkNoEntry();
			break;
		}
		return -1;
	}

	int GetFocusEnterCount(EUxtInteractionMode Mode)
	{
		switch (Mode)
		{
		case EUxtInteractionMode::Near:
			return NearFocusEnterCount;
		case EUxtInteractionMode::Far:
			return FarFocusEnterCount;
		case EUxtInteractionMode::None:
			checkNoEntry();
			break;
		}
		return -1;
	}

	int GetFocusExitCount(EUxtInteractionMode Mode)
	{
		switch (Mode)
		{
		case EUxtInteractionMode::Near:
			return NearFocusExitCount;
		case EUxtInteractionMode::Far:
			return FarFocusExitCount;
		case EUxtInteractionMode::None:
			checkNoEntry();
			break;
		}
		return -1;
	}

	int NearGrabStartCount = 0;
	int NearGrabEndCount = 0;
	int FarGrabStartCount = 0;
	int FarGrabEndCount = 0;

	int NearFocusEnterCount = 0;
	int NearFocusExitCount = 0;
	int FarFocusEnterCount = 0;
	int FarFocusExitCount = 0;
};
