// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "UxtHandMeshCollisionDeactivationComponent.h"

#include "MRMeshComponent.h"

UUxtHandMeshCollisionDeactivationComponent::UUxtHandMeshCollisionDeactivationComponent() : UARTrackableNotifyComponent()
{
	FScriptDelegate OnTrackableUpdatedDelegate;
	OnTrackableUpdatedDelegate.BindUFunction(this, "OnTrackableUpdated");
	OnAddTrackedGeometry.AddUnique(OnTrackableUpdatedDelegate);
	OnUpdateTrackedGeometry.AddUnique(OnTrackableUpdatedDelegate);
}

void UUxtHandMeshCollisionDeactivationComponent::OnTrackableUpdated(UARTrackedGeometry* Updated)
{
	if (Updated->GetObjectClassification() == EARObjectClassification::HandMesh)
	{
		UMRMeshComponent* Mesh = Updated->GetUnderlyingMesh();
		if (IsValid(Mesh))
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}
