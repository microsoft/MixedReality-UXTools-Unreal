// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "OpenXRCommon.h"
#include "SceneUnderstandingBase.h"

namespace MicrosoftOpenXR
{
	class FSpatialMappingPlugin : public FSceneUnderstandingBase
	{
	public:
		FSpatialMappingPlugin();

		IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override;

	protected:
		XrSceneComputeConsistencyMSFT GetSceneComputeConsistency() override;
		TArray<XrSceneComputeFeatureMSFT> GetSceneComputeFeatures(class UARSessionConfig* SessionConfig) override;
	};
}	 // namespace MicrosoftOpenXR
