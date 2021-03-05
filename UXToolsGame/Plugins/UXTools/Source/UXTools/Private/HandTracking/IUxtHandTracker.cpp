// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HandTracking/IUxtHandTracker.h"

#include "Features/IModularFeatures.h"

/* Fallback implementation of the hand tracker interface.
 * In case the modular feature has not been implemented this will ensure a valid singleton reference is returned.
 */
class FDummyHandTracker : public IUxtHandTracker
{
public:
	virtual ETrackingStatus GetTrackingStatus(EControllerHand Hand) const override { return ETrackingStatus::NotTracked; }

	virtual bool IsHandController(EControllerHand Hand) const override { return false; }

	virtual bool GetJointState(
		EControllerHand Hand, EHandKeypoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const override
	{
		return false;
	}

	virtual bool GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const override { return false; }

	virtual bool GetGripPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const override { return false; }

	virtual bool GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const override { return false; }

	virtual bool GetIsSelectPressed(EControllerHand Hand, bool& OutIsSelectPressed) const override { return false; }
};

FName IUxtHandTracker::GetModularFeatureName()
{
	static FName FeatureName = FName(TEXT("UxtHandTracker"));
	return FeatureName;
}

IUxtHandTracker& IUxtHandTracker::Get()
{
	// Fallback implementation if modular feature is not registered
	static FDummyHandTracker DummyHandTracker;

	IModularFeatures& Features = IModularFeatures::Get();
	FName FeatureName = GetModularFeatureName();

	if (Features.IsModularFeatureAvailable(FeatureName))
	{
		return Features.GetModularFeature<IUxtHandTracker>(FeatureName);
	}

	return DummyHandTracker;
}
