// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "UxtPokeTarget.generated.h"

class UPrimitiveComponent;
class UUxtNearPointerComponent;

UENUM(BlueprintType)
enum class EUxtPokeBehaviour : uint8
{
	/** Target represents a plane, only pokable from the front face */
	FrontFace,
	/** Target represents a mesh volume, pokable from all sides */
	Volume,
};

UINTERFACE(BlueprintType)
class UXTOOLS_API UUxtPokeTarget : public UInterface
{
	GENERATED_BODY()
};

/** Interface to implement to enable poke interaction for given primitives. */
class UXTOOLS_API IUxtPokeTarget
{
	GENERATED_BODY()

public:
	/** Returns true if the given primitive should be considerered a valid focus target. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Poke Target")
	bool IsPokeFocusable(const UPrimitiveComponent* Primitive) const;

	/** Returns which poke behaviour this target supports. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Poke Target")
	EUxtPokeBehaviour GetPokeBehaviour() const;

	/** Returns the closest point to Point on the given Primitive. Also provides the surface normal. */
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Poke Target")
	bool GetClosestPoint(const UPrimitiveComponent* Primitive, const FVector& Point, FVector& OutClosestPoint, FVector& OutNormal) const;
};
