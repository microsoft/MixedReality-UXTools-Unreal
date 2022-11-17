// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "HolographicWindowAttachmentPlugin.h"

#if PLATFORM_HOLOLENS
#include "HoloLensApplication.h"

#include <Unknwn.h>

namespace MicrosoftOpenXR
{
	void FHolographicWindowAttachmentPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FHolographicWindowAttachmentPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	bool FHolographicWindowAttachmentPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_HOLOGRAPHIC_WINDOW_ATTACHMENT_EXTENSION_NAME);
		return true;
	}

	const void* FHolographicWindowAttachmentPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		// Ensure xrCreateSession is called from the game thread which is the main app view CoreWindow thread.
		check(IsInGameThread());
		if (IOpenXRHMDModule::Get().IsExtensionEnabled(XR_MSFT_HOLOGRAPHIC_WINDOW_ATTACHMENT_EXTENSION_NAME))
		{
			// If the app requests to start in VR then the XrSession should take over the main app view's CoreWindow and
			// HolographicSpace. This ensures keyboard input and other things will still route to the engine. Otherwise the
			// XrSession will create its own CoreWindow and HolographicSpace and it will take all focus. The HolographicSpace will
			// only be available if start in VR is enabled.
			Windows::Graphics::Holographic::HolographicSpace ^ holographicSpace =
				FHoloLensApplication::GetHoloLensApplication()->GetHolographicSpace();
			if (holographicSpace)
			{
				HolographicWindowAttachment.next = InNext;
				HolographicWindowAttachment.holographicSpace = reinterpret_cast<::IUnknown*>(holographicSpace);
				HolographicWindowAttachment.coreWindow =
					reinterpret_cast<::IUnknown*>(Windows::UI::Core::CoreWindow::GetForCurrentThread());
				return &HolographicWindowAttachment;
			}
		}

		return InNext;
	}
}	 // namespace MicrosoftOpenXR

#endif
