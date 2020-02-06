#pragma once

#include "CoreMinimal.h"

#include "Interactions/UxtGrabbableComponent.h"

#include "GrabbableComponentTickTest.generated.h"

UCLASS()
class UXTOOLSTESTS_API UGrabbableTickTestComponent : public UUxtGrabbableComponent
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
