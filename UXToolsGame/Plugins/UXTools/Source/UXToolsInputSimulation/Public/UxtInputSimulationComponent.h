// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UxtInputSimulationComponent.generated.h"


UCLASS(ClassGroup = UXTools)
class UXTOOLSINPUTSIMULATION_API UUxtInputSimulationComponent
	: public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUxtInputSimulationComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	float MyTestNumber = 1.0f;
};
