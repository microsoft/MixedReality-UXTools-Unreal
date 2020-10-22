#pragma once

#include "OpenXRCommon.h"

namespace MRPlatExt
{
	class FHandInteractionPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void Register();
		void Unregister();

		bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		bool GetInteractionProfile(XrInstance InInstance, FString& OutKeyPrefix, XrPath& OutPath, bool& OutHasHaptics) override;
	};
}	 // namespace MRPlatExt