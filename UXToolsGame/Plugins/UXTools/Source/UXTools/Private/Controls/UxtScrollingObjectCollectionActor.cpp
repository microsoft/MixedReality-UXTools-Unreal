// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#include "Controls/UxtScrollingObjectCollectionActor.h"

#include "UXTools.h"

#include "Controls/UxtScrollingObjectCollectionComponent.h"

AUxtScrollingObjectCollectionActor::AUxtScrollingObjectCollectionActor()
{
	// Components.

	ScrollingObjectCollection = CreateDefaultSubobject<UUxtScrollingObjectCollectionComponent>("ScrollingObjectCollection");
	SetRootComponent(ScrollingObjectCollection);
}
