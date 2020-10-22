#pragma once

#if PLATFORM_HOLOLENS

#include "OpenXRCommon.h"

namespace MRPlatExt
{
	class FHolographicWindowAttachmentPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void Register();
		void Unregister();

		bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;

	private:
		XrHolographicWindowAttachmentMSFT HolographicWindowAttachment{XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT};
	};
}	 // namespace MRPlatExt

#endif