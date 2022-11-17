// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "OpenXRCommon.h"
#include "SceneUnderstandingBase.h"

namespace MicrosoftOpenXR
{
	class FSceneUnderstandingPlugin : public FSceneUnderstandingBase
	{
	public:
		FSceneUnderstandingPlugin();

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		IOpenXRCustomCaptureSupport* GetCustomCaptureSupport(const EARCaptureType CaptureType) override;

		bool OnToggleARCapture(const bool bOnOff) override;

	protected:
		XrSceneComputeConsistencyMSFT GetSceneComputeConsistency() override;
		TArray<XrSceneComputeFeatureMSFT> GetSceneComputeFeatures(class UARSessionConfig* SessionConfig) override;
	};
}	 // namespace MicrosoftOpenXR
