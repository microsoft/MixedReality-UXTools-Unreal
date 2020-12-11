// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Subsystems/LocalPlayerSubsystem.h"

#include "UxtTouchBasedHandTrackerSubsystem.generated.h"

class UWorld;
class APlayerController;
class AGameModeBase;

/**
 * Local player subsystem that adds a UUxtTouchBasedHandTrackerComponent to player controllers as they are created.
 */
UCLASS(ClassGroup = "UXTools|Internal")
class UXTOOLS_API UUxtTouchBasedHandTrackerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	//
	// USubsystem implementation

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);

	FDelegateHandle PostLoginHandle;
};
