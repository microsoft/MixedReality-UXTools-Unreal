#pragma once

#include "CoreMinimal.h"

#include "GrabbableComponent.h"

#include "GrabbableComponentTickTest.generated.h"

UCLASS()
class MIXEDREALITYUTILSTESTS_API UGrabbableTickTestComponent : public UGrabbableComponent
{
	GENERATED_BODY()

public:

	UGrabbableTickTestComponent()
	{
		PrimaryComponentTick.bCanEverTick = true;
	}

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override
	{
		++NumTicks;
	}

	int GetNumTicks() const
	{
		return NumTicks;
	}

	void Reset()
	{
		NumTicks = 0;
	}

private:

	int NumTicks = 0;

};
