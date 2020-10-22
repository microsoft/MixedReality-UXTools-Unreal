#include "OpenXrLoaderPlugin.h"

#if PLATFORM_ANDROID

#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "FMRPlatExtModule"

extern struct android_app* GNativeAndroidApp;

namespace
{
	XRAPI_ATTR XrResult XRAPI_CALL EnumerateInstanceExtensionProperties(
		const char* layerName, uint32_t propertyCapacityInput, uint32_t* propertyCountOutput, XrExtensionProperties* properties)
	{
		if (propertyCapacityInput > 0)
		{
			if (properties == nullptr)
			{
				return XR_ERROR_VALIDATION_FAILURE;
			}
			properties[0].extensionVersion = XR_KHR_android_create_instance_SPEC_VERSION;
			FCStringAnsi::Strncpy(
				properties[0].extensionName, XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME, XR_MAX_EXTENSION_NAME_SIZE);
		}
		if (propertyCountOutput != nullptr)
		{
			*propertyCountOutput = 1;
		}
		return XR_SUCCESS;
	}

	XRAPI_ATTR XrResult XRAPI_CALL GetInstanceProcAddr(XrInstance Instance, const char* Name, PFN_xrVoidFunction* Function)
	{
		if (FCStringAnsi::Strcmp(Name, "xrEnumerateInstanceExtensionProperties") == 0)
		{
			// The current runtime incorrectly returns zero extensions so return a pointer to custom function instead.
			*Function = (PFN_xrVoidFunction) &EnumerateInstanceExtensionProperties;
			return XR_SUCCESS;
		}
		// GetDllHandle will return the handle if the library was already loaded.
		// This whole function (GetInstanceProcAddr) is temporary until the loader's xrGetInstanceProcAddr function is fixed.
		void* Handle = FPlatformProcess::GetDllHandle(TEXT("libopenxr_loader.so"));
		if (Handle == nullptr)
		{
			return XR_ERROR_RUNTIME_FAILURE;
		}
		PFN_xrVoidFunction Func = (PFN_xrVoidFunction) FPlatformProcess::GetDllExport(Handle, ANSI_TO_TCHAR(Name));
		if (Func == nullptr)
		{
			return XR_ERROR_FUNCTION_UNSUPPORTED;
		}
		*Function = Func;
		return XR_SUCCESS;
	}
}	 // namespace

namespace MRPlatExt
{
	FOpenXrLoaderPlugin::~FOpenXrLoaderPlugin()
	{
		FreeLoader();
	}

	void FOpenXrLoaderPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FOpenXrLoaderPlugin::Unregister()
	{
		FreeLoader();

		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	bool FOpenXrLoaderPlugin::GetCustomLoader(PFN_xrGetInstanceProcAddr* OutGetProcAddr)
	{
		if (LoaderHandle == nullptr)
		{
			// GetDllHandle will log if there is an error
			LoaderHandle = FPlatformProcess::GetDllHandle(TEXT("libopenxr_loader.so"));
			if (LoaderHandle == nullptr)
			{
				return false;
			}
		}
		// The current runtime's xrGetInstanceProcAddr doesn't return valid pointers so use an alternative.
		*OutGetProcAddr = (PFN_xrGetInstanceProcAddr) &GetInstanceProcAddr;
		return true;
	}

	bool FOpenXrLoaderPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME);
		return true;
	}

	const void* FOpenXrLoaderPlugin::OnCreateInstance(class IOpenXRHMDPlugin* InPlugin, const void* InNext)
	{
		AndroidCreateInfo.applicationVM = GNativeAndroidApp->activity->vm;
		AndroidCreateInfo.applicationActivity = GNativeAndroidApp->activity->clazz;
		AndroidCreateInfo.next = InNext;
		return &AndroidCreateInfo;
	}

	void FOpenXrLoaderPlugin::FreeLoader()
	{
		if (LoaderHandle != nullptr)
		{
			FPlatformProcess::FreeDllHandle(LoaderHandle);
			LoaderHandle = nullptr;
		}
	}

}	 // namespace MRPlatExt
#endif
