#pragma once
#if PLATFORM_ANDROID
#include "OpenXRCommon.h"

namespace MRPlatExt
{
	class FOpenXrLoaderPlugin : public IOpenXRExtensionPlugin
	{
	public:
		~FOpenXrLoaderPlugin() override;
		void Register();
		void Unregister();

		bool GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr) override;
		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateInstance(class IOpenXRHMDPlugin* InPlugin, const void* InNext) override;

	private:
		void FreeLoader();

		void* LoaderHandle{nullptr};
		XrInstanceCreateInfoAndroidKHR AndroidCreateInfo{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
	};
}	 // namespace MRPlatExt
#endif
