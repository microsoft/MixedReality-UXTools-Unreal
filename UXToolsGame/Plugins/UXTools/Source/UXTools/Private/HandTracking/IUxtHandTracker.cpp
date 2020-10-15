// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/IUxtHandTracker.h"

#include "Features/IModularFeatures.h"

FName IUxtHandTracker::GetModularFeatureName()
{
	static FName FeatureName = FName(TEXT("UxtHandTracker"));
	return FeatureName;
}

IUxtHandTracker* IUxtHandTracker::GetHandTracker()
{
	IModularFeatures& Features = IModularFeatures::Get();
	FName FeatureName = GetModularFeatureName();

	if (Features.IsModularFeatureAvailable(FeatureName))
	{
		return &Features.GetModularFeature<IUxtHandTracker>(FeatureName);
	}

	return nullptr;
}
