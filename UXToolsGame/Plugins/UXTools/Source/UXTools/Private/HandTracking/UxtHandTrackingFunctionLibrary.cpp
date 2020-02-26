// Fill out your copyright notice in the Description page of Project Settings.


#include "HandTracking/UxtHandTrackingFunctionLibrary.h"
#include "Features/IModularFeatures.h"

IUxtHandTracker* UUxtHandTrackingFunctionLibrary::GetHandTracker()
{
	IModularFeatures& Features = IModularFeatures::Get();
	FName FeatureName = IUxtHandTracker::GetModularFeatureName();

	if (Features.IsModularFeatureAvailable(FeatureName))
	{
		return &Features.GetModularFeature<IUxtHandTracker>(FeatureName);
	}

	return nullptr;
}

bool UUxtHandTrackingFunctionLibrary::GetHandJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius)
{
	if (IUxtHandTracker* HandTracker = GetHandTracker())
	{
		return HandTracker->GetJointState(Hand, Joint, OutOrientation, OutPosition, OutRadius);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::GetIsHandGrabbing(EControllerHand Hand, bool& OutIsGrabbing)
{
	if (IUxtHandTracker* HandTracker = GetHandTracker())
	{
		return HandTracker->GetIsGrabbing(Hand, OutIsGrabbing);
	}

	return false;
}

bool UUxtHandTrackingFunctionLibrary::IsHandTracked(EControllerHand Hand)
{
	bool NotUsed = false;
	return GetIsHandGrabbing(Hand, NotUsed);
}