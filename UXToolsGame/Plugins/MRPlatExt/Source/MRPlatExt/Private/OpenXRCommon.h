#pragma once

#include <openxr/openxr.h>

#if PLATFORM_ANDROID
#define XR_USE_PLATFORM_ANDROID 1
#include <jni.h>
#include <android_native_app_glue.h>
#endif

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#define XR_USE_PLATFORM_WIN32 1
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#include <openxr/openxr_platform.h>

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include "IOpenXRHMDPlugin.h"
#include "IOpenXRExtensionPlugin.h"

namespace MRPlatExt
{
	XrPath GetXrPath(XrInstance Instance, const char* PathString);
}
