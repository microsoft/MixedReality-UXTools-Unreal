#include "HandInteractionPlugin.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "FMRPlatExtModule"

namespace
{
	const FKey MicrosoftHandInteraction_Left_Select("MicrosoftHandInteraction_Left_Select_Axis");
	const FKey MicrosoftHandInteraction_Left_Squeeze("MicrosoftHandInteraction_Left_Squeeze_Axis");
	const FKey MicrosoftHandInteraction_Right_Select("MicrosoftHandInteraction_Right_Select_Axis");
	const FKey MicrosoftHandInteraction_Right_Squeeze("MicrosoftHandInteraction_Right_Squeeze_Axis");
}	 // namespace

namespace MRPlatExt
{
	bool FHandInteractionPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME);
		return true;
	}

	bool FHandInteractionPlugin::GetInteractionProfile(
		XrInstance InInstance, FString& OutKeyPrefix, XrPath& OutPath, bool& OutHasHaptics)
	{
		if (!IOpenXRHMDPlugin::Get().IsExtensionEnabled(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME))
		{
			return false;
		}

		OutKeyPrefix = "MicrosoftHandInteraction";
		OutPath = GetXrPath(InInstance, "/interaction_profiles/microsoft/hand_interaction");
		OutHasHaptics = false;
		return true;
	}

	void FHandInteractionPlugin::Register()
	{
		EKeys::AddMenuCategoryDisplayInfo("MicrosoftHandInteraction",
			LOCTEXT("MicrosoftHandInteractionSubCategory", "Microsoft Hand Interaction"), TEXT("GraphEditor.PadEvent_16x"));
		EKeys::AddKey(FKeyDetails(MicrosoftHandInteraction_Left_Select,
			LOCTEXT("MicrosoftHandInteraction_Left_Select_Axis", "Microsoft Hand (L) Select"),
			FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MicrosoftHandInteraction"));
		EKeys::AddKey(FKeyDetails(MicrosoftHandInteraction_Left_Squeeze,
			LOCTEXT("MicrosoftHandInteraction_Left_Squeeze_Axis", "Microsoft Hand (L) Squeeze"),
			FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MicrosoftHandInteraction"));
		EKeys::AddKey(FKeyDetails(MicrosoftHandInteraction_Right_Select,
			LOCTEXT("MicrosoftHandInteraction_Right_Select_Axis", "Microsoft Hand (R) Select"),
			FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MicrosoftHandInteraction"));
		EKeys::AddKey(FKeyDetails(MicrosoftHandInteraction_Right_Squeeze,
			LOCTEXT("MicrosoftHandInteraction_Right_Squeeze_Axis", "Microsoft Hand (R) Squeeze"),
			FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "MicrosoftHandInteraction"));

		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FHandInteractionPlugin::Unregister()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}
}	 // namespace MRPlatExt