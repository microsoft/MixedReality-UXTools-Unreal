// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if PLATFORM_HOLOLENS

#include "OpenXRCommon.h"

namespace MicrosoftOpenXR
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
}	 // namespace MicrosoftOpenXR

#endif
