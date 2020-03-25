// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UxtFarTarget.generated.h"

USTRUCT(BlueprintType)
struct UXTOOLS_API FUxtFarFocusEvent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Focus")
	UPrimitiveComponent* Primitive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Focus")
	FVector HitPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Far Focus")
	FVector HitNormal;
};

UINTERFACE(BlueprintType)
class UUxtFarTarget : public UInterface
{
	GENERATED_BODY()
};

class UUxtFarPointerComponent;

class UXTOOLS_API IUxtFarTarget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent)
	void OnEnterFarFocus(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent);

	UFUNCTION(BlueprintNativeEvent)
	void OnUpdatedFarFocus(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent);

	UFUNCTION(BlueprintNativeEvent)
	void OnExitFarFocus(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent);

	UFUNCTION(BlueprintNativeEvent)
	void OnFarPressed(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent);

	UFUNCTION(BlueprintNativeEvent)
	void OnFarReleased(UUxtFarPointerComponent* Pointer, const FUxtFarFocusEvent& FarFocusEvent);
};
