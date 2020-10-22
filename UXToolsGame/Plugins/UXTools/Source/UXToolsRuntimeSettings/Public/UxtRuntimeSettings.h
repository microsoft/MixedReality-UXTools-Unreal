// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "UObject/Object.h"

#include "UxtRuntimeSettings.generated.h"

class UAnimInstance;
class USkeletalMesh;

/**
 * Settings for UXTools.
 */
UCLASS(config = EditorPerProjectUserSettings)
class UXTOOLSRUNTIMESETTINGS_API UUxtRuntimeSettings : public UObject
{
public:
	GENERATED_BODY()

	UUxtRuntimeSettings(const FObjectInitializer& ObjectInitializer);

	static UUxtRuntimeSettings* Get();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

private:
	static class UUxtRuntimeSettings* UXToolsSettingsSingleton;
};
