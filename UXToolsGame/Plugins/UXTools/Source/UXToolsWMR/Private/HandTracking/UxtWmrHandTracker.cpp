#include "HandTracking/UxtWmrHandTracker.h"
#include "WindowsMixedRealityHandTrackingFunctionLibrary.h"
#include "WindowsMixedRealityFunctionLibrary.h"

#if !PLATFORM_HOLOLENS
#include "WindowsMixedRealityInputSimulationEngineSubsystem.h"
#endif


bool FUxtWmrHandTracker::GetJointState(EControllerHand Hand, EUxtHandJoint Joint, FQuat& OutOrientation, FVector& OutPosition, float& OutRadius)
{
	if (Joint == EUxtHandJoint::Pointer)
	{
		FPointerPoseInfo Info = UWindowsMixedRealityFunctionLibrary::GetPointerPoseInfo(Hand);
		if (Info.TrackingStatus != EHMDTrackingStatus::NotTracked)
		{
			OutOrientation = Info.Orientation;
			OutPosition = Info.Origin;
			OutRadius = 0.0f;
			return true;
		}
	}
	else
	{
		EWMRHandKeypoint Keypoint = (EWMRHandKeypoint)Joint;
		FTransform Transform;
		if (UWindowsMixedRealityHandTrackingFunctionLibrary::GetHandJointTransform(Hand, Keypoint, Transform, OutRadius))
		{
			OutOrientation = Transform.GetRotation();
			OutPosition = Transform.GetTranslation();
			return true;
		}
	}

	return false;
}

bool FUxtWmrHandTracker::GetIsGrabbing(EControllerHand Hand, bool& OutIsGrabbing)
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