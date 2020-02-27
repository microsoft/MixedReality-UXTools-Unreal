#include "HandTracking/UxtWmrHandTracker.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"
#include "WindowsMixedRealityFunctionLibrary.h"
#include "Utils/UxtFunctionLibrary.h"

#if !PLATFORM_HOLOLENS
#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"
#endif


bool FUxtWmrHandTracker::GetJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius) const
{
	EWMRHandKeypoint Keypoint = (EWMRHandKeypoint)Joint;
	FTransform Transform;

	if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Keypoint, Transform, OutRadius))
	{
		OutOrientation = Transform.GetRotation();
		OutPosition = Transform.GetTranslation();
		return true;
	}

	return false;
}

bool FUxtWmrHandTracker::GetPointerPose(EControllerHand Hand, FQuat& OutOrientation, FVector& OutPosition) const
{
	if (UUxtFunctionLibrary::IsInEditor())
	{
		// Simulate pointer pose using the wrist. Workaround BUG 233.
		FTransform Transform;
		float Radius;
		UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, EWMRHandKeypoint::Wrist, Transform, Radius);

		OutPosition = Transform.GetTranslation();
		OutOrientation = Transform.GetRotation()* FQuat(FVector::RightVector, 0.75f * HALF_PI);
	}
	else
	{
		FPointerPoseInfo Info = UWindowsMixedRealityFunctionLibrary::GetPointerPoseInfo(Hand);

		if (Info.TrackingStatus != EHMDTrackingStatus::NotTracked)
		{
			OutOrientation = Info.Orientation;
			OutPosition = Info.Origin;
			return true;
		}
	}

	return false;
}

bool FUxtWmrHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing) const
{
	bool bTracked;

	// Workaround BUG 141
#if !PLATFORM_HOLOLENS
	if (UWindowsMixedRealityInputSimulationEngineSubsystem* InputSim = UWindowsMixedRealityInputSimulationEngineSubsystem::GetInputSimulationIfEnabled())
	{
		bTracked = InputSim->GetControllerTrackingStatus(Hand) != ETrackingStatus::NotTracked;
	}
	else
#endif
	{
		bTracked = UWindowsMixedRealityFunctionLibrary::GetControllerTrackingStatus(Hand) != EHMDTrackingStatus::NotTracked;
	}

	if (bTracked)
	{
		OutIsGrabbing = UWindowsMixedRealityFunctionLibrary::IsGrasped(Hand);
	}

	return bTracked;
}