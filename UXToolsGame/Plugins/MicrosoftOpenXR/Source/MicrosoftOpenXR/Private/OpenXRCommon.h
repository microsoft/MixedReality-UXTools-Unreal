// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <openxr_private/openxr.h>

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#pragma warning(disable : 5205 4265 4268 4946)

#define XR_USE_PLATFORM_WIN32 1
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <unknwn.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#endif

#include <openxr_private/openxr_platform.h>

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "IOpenXRHMDModule.h"
#include "IOpenXRARModule.h"
#include "IOpenXRExtensionPlugin.h"

namespace MicrosoftOpenXR
{
#if DO_CHECK
	// Modified from OpenXRCore's XR_ENSURE which uses engine OpenXR headers to get a result string.
	// Otherwise, this will fail to build when using newer OpenXR headers in this plugin.
#define XR_ENSURE_MSFT(x) [] (XrResult Result) \
	{ \
		return ensureMsgf(XR_SUCCEEDED(Result), TEXT("OpenXR call failed with result: %d"), Result); \
	} (x)
#else
#define XR_ENSURE_MSFT(x) XR_SUCCEEDED(x)
#endif

	XrPath GetXrPath(XrInstance Instance, const char* PathString);
}
