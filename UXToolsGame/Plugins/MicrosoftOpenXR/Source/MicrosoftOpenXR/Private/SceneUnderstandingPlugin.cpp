// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#include "SceneUnderstandingPlugin.h"

namespace MicrosoftOpenXR
{
	FSceneUnderstandingPlugin::FSceneUnderstandingPlugin()
	{
	}

	bool FSceneUnderstandingPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		return FSceneUnderstandingBase::GetRequiredExtensions(OutExtensions);
	}

	IOpenXRCustomCaptureSupport* FSceneUnderstandingPlugin::GetCustomCaptureSupport(const EARCaptureType CaptureType)
	{
		return CaptureType == EARCaptureType::SceneUnderstanding ? this : nullptr;
	}

	bool FSceneUnderstandingPlugin::OnToggleARCapture(const bool bOnOff)
	{
		if (bOnOff && !CanDetectPlanes())
		{
			UE_LOG(LogHMD, Warning, TEXT("Attempting to use scene understanding in a runtime that does not support plane finding.  Use spatial mapping instead."));
			return false;
		}

		return FSceneUnderstandingBase::OnToggleARCapture(bOnOff);
	}

	XrSceneComputeConsistencyMSFT FSceneUnderstandingPlugin::GetSceneComputeConsistency()
	{
		return XR_SCENE_COMPUTE_CONSISTENCY_SNAPSHOT_COMPLETE_MSFT;
	}

	TArray<XrSceneComputeFeatureMSFT> FSceneUnderstandingPlugin::GetSceneComputeFeatures(class UARSessionConfig* SessionConfig)
	{
		bool bGenerateSceneMeshData = false;
		GConfig->GetBool(TEXT("/Script/HoloLensSettings.SceneUnderstanding"), TEXT("ShouldDoSceneUnderstandingMeshDetection"),
			bGenerateSceneMeshData, GGameIni);

		//TODO: Restore this block when the session config exposes this flag (UE-126562). Update version when known.
		//#if !UE_VERSION_OLDER_THAN(4, 27, 2)
		//if (!bGenerateSceneMeshData)
		//{
		//	// Game ini does not have mesh detection flag set, check session config
		//	bGenerateSceneMeshData = SessionConfig->ShouldDoSceneUnderstandingMeshDetection();
		//}
		//#endif

		TArray<XrSceneComputeFeatureMSFT> SceneComputeFeatures;
		SceneComputeFeatures.AddUnique(XR_SCENE_COMPUTE_FEATURE_PLANE_MSFT);

		if (bGenerateSceneMeshData)
		{
			SceneComputeFeatures.AddUnique(XR_SCENE_COMPUTE_FEATURE_PLANE_MESH_MSFT);
		}

		return SceneComputeFeatures;
	}

}	 // namespace MicrosoftOpenXR
