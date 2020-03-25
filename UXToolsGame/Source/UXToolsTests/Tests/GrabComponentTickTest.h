#pragma once

#include "CoreMinimal.h"

#include "Interactions/UxtGrabTargetComponent.h"

#include "GrabComponentTickTest.generated.h"

UCLASS()
class UXTOOLSTESTS_API UGrabTickTestComponent : public UUxtGrabTargetComponent
{
	GENERATED_BODY()

public:

	UGrabTickTestComponent()
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
