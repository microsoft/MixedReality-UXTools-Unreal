// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ARTrackableNotifyComponent.h"
#include "CoreMinimal.h"

#include "UxtHandMeshCollisionDeactivationComponent.generated.h"

/**
 * Internal component used for deactivating collision on Hand Mesh which is necessary when Collision Depth Data is enabled in AR Session.
 * It is meant to be used internaly by the UxtHandInteractionActor and is likely to be removed from UX Tools after a fix
 * is provided in future versions of the Unreal Engine / UX Tools.
 */
UCLASS(ClassGroup = "UXTools")
class UXTOOLS_API UUxtHandMeshCollisionDeactivationComponent : public UARTrackableNotifyComponent
{
	GENERATED_BODY()

public:
	UUxtHandMeshCollisionDeactivationComponent();

private:
	UFUNCTION()
	void OnTrackableUpdated(UARTrackedGeometry* Updated);
};