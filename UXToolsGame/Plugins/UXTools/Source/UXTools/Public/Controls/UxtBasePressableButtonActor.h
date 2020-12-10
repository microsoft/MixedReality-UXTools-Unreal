// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Controls/UxtCollectionObject.h"
#include "GameFramework/Actor.h"

#include "UxtBasePressableButtonActor.generated.h"

class UUxtPressableButtonComponent;

/**
 * Abstract base class for all pressable button actors. Ensures a UUxtPressableButtonComponent as the root component.
 */
UCLASS(Abstract, ClassGroup = "UXTools")
class UXTOOLS_API AUxtBasePressableButtonActor
	: public AActor
	, public IUxtCollectionObject
{
	GENERATED_BODY()

public:
	AUxtBasePressableButtonActor();

	//
	// IUxtCollectionObject interface

	/** Returns UUxtPressableButtonComponent as the poke target. **/
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Pressable Button")
	TScriptInterface<IUxtPokeTarget> GetPokeTarget();
	virtual TScriptInterface<IUxtPokeTarget> GetPokeTarget_Implementation() override;

	/** Returns UUxtPressableButtonComponent as the far target. **/
	UFUNCTION(BlueprintNativeEvent, Category = "Uxt Pressable Button")
	TScriptInterface<IUxtFarTarget> GetFarTarget();
	virtual TScriptInterface<IUxtFarTarget> GetFarTarget_Implementation() override;

	//
	// AUxtBasePressableButtonActor interface

	/** Returns UUxtPressableButtonComponent subobject. **/
	UUxtPressableButtonComponent* GetButtonComponent() const { return ButtonComponent; }

protected:
	/** Handle to the root button component. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Uxt Pressable Button", meta = (AllowPrivateAccess = "true"))
	UUxtPressableButtonComponent* ButtonComponent = nullptr;
};
