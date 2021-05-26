// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "UxtScrollingObjectCollectionActor.generated.h"

class UUxtScrollingObjectCollectionComponent;

/**
 * The default scrolling object collection actor which programmatically builds a scrolling object collection actor.
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API AUxtScrollingObjectCollectionActor : public AActor
{
	GENERATED_BODY()

public:
	AUxtScrollingObjectCollectionActor();

	//
	// AUxtScrollingObjectCollectionActor interface

	/** Returns UUxtScrollingObjectCollectionComponent subobject. **/
	UFUNCTION(Category = "Uxt Scrolling Object Collection")
	UUxtScrollingObjectCollectionComponent* GetScrollingObjectCollectionComponent() const { return ScrollingObjectCollection; }

protected:
	//
	// Components.

	/** The scrolling object collection functionality. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Scrolling Object Collection")
	UUxtScrollingObjectCollectionComponent* ScrollingObjectCollection = nullptr;

private:
};
