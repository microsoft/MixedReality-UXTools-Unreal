// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "OpenXRCommon.h"
#include "CoreMinimal.h"
#include "OpenXRCore.h"

namespace MicrosoftOpenXR
{
	XrPath GetXrPath(XrInstance Instance, const char* PathString)
	{
		XrPath Path = XR_NULL_PATH;
		XrResult Result = xrStringToPath(Instance, PathString, &Path);
		check(XR_SUCCEEDED(Result));
		return Path;
	}
}	 // namespace MicrosoftOpenXR
