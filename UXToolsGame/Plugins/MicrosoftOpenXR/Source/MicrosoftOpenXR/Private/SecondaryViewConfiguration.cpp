#include "SecondaryViewConfiguration.h"

#include "MicrosoftOpenXR.h"
#include "OpenXRCore.h"

namespace
{
	TArray<XrViewConfigurationType> PluginSupportedSecondaryViewConfigTypes = {
		XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT};

#define XR_ENUM_CASE_STR(name, val) \
	case name:                      \
		return TEXT(#name);
	constexpr const TCHAR* ViewConfigTypeToString(XrViewConfigurationType v)
	{
		switch (v)
		{
			XR_LIST_ENUM_XrViewConfigurationType(XR_ENUM_CASE_STR) default : return TEXT("Unknown");
		}
	}
}	 // namespace

namespace MicrosoftOpenXR
{
	void FSecondaryViewConfigurationPlugin::Register()
	{
		// Secondary view feature can trigger engine bug in 5.0 so this plugin is disabled until it is fixed.
		// IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void FSecondaryViewConfigurationPlugin::Unregister()
	{
		// Secondary view feature can trigger engine bug in 5.0 so this plugin is disabled until it is fixed.
		// IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	bool FSecondaryViewConfigurationPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SECONDARY_VIEW_CONFIGURATION_EXTENSION_NAME);
		return true;
	}

	bool FSecondaryViewConfigurationPlugin::GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_FIRST_PERSON_OBSERVER_EXTENSION_NAME);
		return true;
	}

	void FSecondaryViewConfigurationPlugin::PostGetSystem(XrInstance InInstance, XrSystemId InSystem)
	{
		Instance = InInstance;
		System = InSystem;
	}

	const void* FSecondaryViewConfigurationPlugin::OnBeginSession(XrSession InSession, const void* InNext)
	{
		check(Instance != XR_NULL_HANDLE);
		check(System != XR_NULL_SYSTEM_ID);

		uint32_t ConfigurationCount;
		XR_ENSURE_MSFT(xrEnumerateViewConfigurations(Instance, System, 0, &ConfigurationCount, nullptr));
		TArray<XrViewConfigurationType> AvailableViewConfigTypes;
		AvailableViewConfigTypes.SetNum(ConfigurationCount);
		XR_ENSURE_MSFT(xrEnumerateViewConfigurations(
			Instance, System, ConfigurationCount, &ConfigurationCount, AvailableViewConfigTypes.GetData()));

		// Generate the overlap of the view configuration types supported by this plugin and the runtime and
		// set up some of the core structs.
		EnabledViewConfigTypes.Reset();
		EnabledViewConfigEnvBlendModes.Reset();
		SecondaryViewState_GameThread.SecondaryViewConfigStates.Reset();
		EnabledViewConfigurationViews.Reset();
		for (XrViewConfigurationType ViewConfigType : PluginSupportedSecondaryViewConfigTypes)
		{
			if (!AvailableViewConfigTypes.Contains(ViewConfigType))
			{
				continue;	 // Runtime doesn't support this secondary view config type.
			}

			EnabledViewConfigTypes.Add(ViewConfigType);

			// Store the corresponding blend mode to use for this view configuration type.
			uint32_t EnvBlendModeCount = 0;
			XR_ENSURE_MSFT(xrEnumerateEnvironmentBlendModes(Instance, System, ViewConfigType, 0, &EnvBlendModeCount, nullptr));
			TArray<XrEnvironmentBlendMode> EnvBlendModes;
			EnvBlendModes.SetNum(EnvBlendModeCount);
			XR_ENSURE_MSFT(xrEnumerateEnvironmentBlendModes(
				Instance, System, ViewConfigType, EnvBlendModeCount, &EnvBlendModeCount, EnvBlendModes.GetData()));
			EnabledViewConfigEnvBlendModes.Add(EnvBlendModes[0]);

			XrSecondaryViewConfigurationStateMSFT viewState = {XR_TYPE_SECONDARY_VIEW_CONFIGURATION_STATE_MSFT};
			viewState.viewConfigurationType = ViewConfigType;
			SecondaryViewState_GameThread.SecondaryViewConfigStates.Add(viewState);

			// Enumerate the view configuration's views
			uint32_t ViewConfigCount = 0;
			XR_ENSURE_MSFT(xrEnumerateViewConfigurationViews(Instance, System, ViewConfigType, 0, &ViewConfigCount, nullptr));
			TArray<XrViewConfigurationView> Views;
			Views.SetNum(ViewConfigCount);
			for (uint32_t i = 0; i < ViewConfigCount; i++)
			{
				Views[i] = {XR_TYPE_VIEW_CONFIGURATION_VIEW};
			}
			XR_ENSURE_MSFT(xrEnumerateViewConfigurationViews(
				Instance, System, ViewConfigType, ViewConfigCount, &ViewConfigCount, Views.GetData()));
			EnabledViewConfigurationViews.Add(std::move(Views));
		}

		// It is only legal to chain in the secondary view configuration information if there is one or more supported secondary
		// view configurations being enabled.
		if (EnabledViewConfigTypes.Num() == 0)
		{
			return InNext;
		}

		SecondaryViewConfigurationSessionBeginInfo.type = XR_TYPE_SECONDARY_VIEW_CONFIGURATION_SESSION_BEGIN_INFO_MSFT;
		SecondaryViewConfigurationSessionBeginInfo.next = InNext;
		SecondaryViewConfigurationSessionBeginInfo.viewConfigurationCount = EnabledViewConfigTypes.Num();
		SecondaryViewConfigurationSessionBeginInfo.enabledViewConfigurationTypes = EnabledViewConfigTypes.GetData();
		return &SecondaryViewConfigurationSessionBeginInfo;
	}

	void* FSecondaryViewConfigurationPlugin::OnWaitFrame(XrSession InSession, void* InNext)
	{
		// If there are no enabled seconary view configs there is no need to query their state.
		if (EnabledViewConfigTypes.Num() == 0)
		{
			return InNext;
		}

		check(IsInGameThread());
		check(SecondaryViewState_GameThread.SecondaryViewConfigStates.Num() == EnabledViewConfigTypes.Num());

		SecondaryViewConfigurationFrameState.type = XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_STATE_MSFT;
		SecondaryViewConfigurationFrameState.next = InNext;
		SecondaryViewConfigurationFrameState.viewConfigurationCount = SecondaryViewState_GameThread.SecondaryViewConfigStates.Num();
		SecondaryViewConfigurationFrameState.viewConfigurationStates =
			SecondaryViewState_GameThread.SecondaryViewConfigStates.GetData();
		return &SecondaryViewConfigurationFrameState;
	}

	const void* FSecondaryViewConfigurationPlugin::OnBeginFrame(XrSession InSession, XrTime DisplayTime, const void* InNext)
	{
		// Log when the active state of a secondary view config changes. Ideally this would be done immediately after xrWaitFrame
		// completes but there is no "PostWaitFrame" callback.
		const int SharedViewCount = FMath::Min(SecondaryViewState_RenderThread.SecondaryViewConfigStates.Num(),
			SecondaryViewState_GameThread.SecondaryViewConfigStates.Num());
		for (int ViewIndex = 0; ViewIndex < SharedViewCount; ViewIndex++)
		{
			const XrSecondaryViewConfigurationStateMSFT& RenderViewState =
				SecondaryViewState_RenderThread.SecondaryViewConfigStates[ViewIndex];
			const XrSecondaryViewConfigurationStateMSFT& GameViewState =
				SecondaryViewState_GameThread.SecondaryViewConfigStates[ViewIndex];
			if (GameViewState.active != RenderViewState.active)
			{
				UE_LOG(LogHMD, Log, TEXT("Secondary view configuration %s changed to %s"),
					ViewConfigTypeToString(RenderViewState.viewConfigurationType),
					RenderViewState.active ? TEXT("active") : TEXT("inactive"));
			}
		}

		// xrBeginFrame corresponds to the previous xrWaitFrame. After xrBeginFrame completes (after this callback is completed), a
		// subsequent xrWaitFrame can begin. Because xrBeginFrame acts as a synchronization point with xrWaitFrame, no lock is
		// needed to clone over state for the rendering operations.
		SecondaryViewState_RenderThread = SecondaryViewState_GameThread;

		return InNext;
	}

	void FSecondaryViewConfigurationPlugin::GetViewConfigurations(XrSystemId InSystem, TArray<XrViewConfigurationView>& OutViews)
	{
		PiplinedFrameState& ViewConfigurationFrameState = GetSecondaryViewStateForThread();

		// Give back the views enabled for this frame.
		for (int i = 0; i < ViewConfigurationFrameState.SecondaryViewConfigStates.Num(); i++)
		{
			if (ViewConfigurationFrameState.SecondaryViewConfigStates[i].active)
			{
				// Append all of the views for this active view configuration.
				OutViews.Append(EnabledViewConfigurationViews[i]);
			}
		}
	}

	void FSecondaryViewConfigurationPlugin::GetViewLocations(
		XrSession InSession, XrTime InDisplayTime, XrSpace InSpace, TArray<XrView>& OutViews)
	{
		PiplinedFrameState& ViewConfigurationFrameState = GetSecondaryViewStateForThread();

		ViewConfigurationFrameState.ViewSpace = InSpace;

		// Return back the views enabled for this frame.
		ViewConfigurationFrameState.SecondaryViews.SetNum(ViewConfigurationFrameState.SecondaryViewConfigStates.Num());
		for (int i = 0; i < ViewConfigurationFrameState.SecondaryViewConfigStates.Num(); i++)
		{
			TArray<XrView>& SecondaryViews = ViewConfigurationFrameState.SecondaryViews[i];

			SecondaryViews.Reset();
			if (ViewConfigurationFrameState.SecondaryViewConfigStates[i].active)
			{
				SecondaryViews.SetNumUninitialized(EnabledViewConfigurationViews[i].Num());
				for (XrView& view : SecondaryViews)
				{
					view = {XR_TYPE_VIEW};
				}

				XrViewLocateInfo ViewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
				ViewLocateInfo.displayTime = InDisplayTime;
				ViewLocateInfo.space = InSpace;
				ViewLocateInfo.viewConfigurationType = EnabledViewConfigTypes[i];

				XrViewState ViewState{XR_TYPE_VIEW_STATE};
				uint32_t ViewOutput;
				XR_ENSURE_MSFT(xrLocateViews(
					InSession, &ViewLocateInfo, &ViewState, SecondaryViews.Num(), &ViewOutput, SecondaryViews.GetData()));
				check(ViewOutput == SecondaryViews.Num());
				check(ViewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT);
				check(ViewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT);
				OutViews.Append(SecondaryViews);
			}
		}
	}

	const void* FSecondaryViewConfigurationPlugin::OnEndFrame(XrSession InSession, XrTime DisplayTime,
		const TArray<XrSwapchainSubImage> InColorImages, const TArray<XrSwapchainSubImage> InDepthImages, const void* InNext)
	{
		const PiplinedFrameState& ViewConfigurationFrameState = GetSecondaryViewStateForThread();

		SecondaryProjectionLayers.Reset();

		// Images are provided flattened, so this counter increments for every image across all layers.
		int AllViewLocationsIndex = 0;

		ensure(EnabledViewConfigTypes.Num() == EnabledViewConfigEnvBlendModes.Num());
		ensure(ViewConfigurationFrameState.SecondaryViewConfigStates.Num() == EnabledViewConfigEnvBlendModes.Num());
		for (int i = 0; i < ViewConfigurationFrameState.SecondaryViewConfigStates.Num(); i++)
		{
			const XrSecondaryViewConfigurationStateMSFT& ViewConfigState = ViewConfigurationFrameState.SecondaryViewConfigStates[i];
			if (!ViewConfigState.active)
			{
				continue;
			}

			const TArray<XrView>& SecondaryViews = ViewConfigurationFrameState.SecondaryViews[i];

			TArray<XrCompositionLayerProjectionView> ProjectionViews;
			for (int ViewIndex = 0; ViewIndex < SecondaryViews.Num(); ViewIndex++)
			{
				// InColorImages should be a 1:1 match with the SecondaryViews.
				if (AllViewLocationsIndex >= InColorImages.Num() ||
					InColorImages[AllViewLocationsIndex].swapchain == XR_NULL_HANDLE)
				{
					// The allocation of the swapchain takes one frame to bring up so this is expected for one frame.
					UE_LOG(LogHMD, Warning, TEXT("No ColorImage available for active secondary view configuration %s"),
						ViewConfigTypeToString(ViewConfigState.viewConfigurationType));
					break;
				}

				XrCompositionLayerProjectionView ProjectionView{XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
				ProjectionView.fov = SecondaryViews[ViewIndex].fov;
				ProjectionView.pose = SecondaryViews[ViewIndex].pose;
				ProjectionView.subImage = InColorImages[AllViewLocationsIndex];
				ProjectionViews.Add(ProjectionView);

				AllViewLocationsIndex++;
			}

			if (ProjectionViews.Num() != SecondaryViews.Num())
			{
				// If something went wrong and the earlier loop wasn't able to set up all of the projection views then throw the
				// data away.
				continue;
			}

			SecondaryProjectionLayers.Add(SingleProjectionLayer(ViewConfigState.viewConfigurationType,
				EnabledViewConfigEnvBlendModes[i], ViewConfigurationFrameState.ViewSpace, std::move(ProjectionViews)));
		}

		// Now that the SecondaryProjectionLayers TArray is completed and there is no more chance of resize, set up the
		// collection of XrSecondaryViewConfigurationLayerInfoMSFT.
		SecondaryViewLayers.SetNum(SecondaryProjectionLayers.Num());
		for (int i = 0; i < SecondaryProjectionLayers.Num(); i++)
		{
			SecondaryProjectionLayers[i].Finalize(&SecondaryViewLayers[i]);
		}

		if (SecondaryViewLayers.Num() > 0)
		{
			SecondaryViewConfigurationEndInfo = {XR_TYPE_SECONDARY_VIEW_CONFIGURATION_FRAME_END_INFO_MSFT, InNext};
			SecondaryViewConfigurationEndInfo.viewConfigurationCount = SecondaryViewLayers.Num();
			SecondaryViewConfigurationEndInfo.viewConfigurationLayersInfo = SecondaryViewLayers.GetData();
			return &SecondaryViewConfigurationEndInfo;
		}

		return InNext;
	}

	FSecondaryViewConfigurationPlugin::PiplinedFrameState& FSecondaryViewConfigurationPlugin::GetSecondaryViewStateForThread()
	{
		if (IsInGameThread())
		{
			return SecondaryViewState_GameThread;
		}
		else
		{
			check(IsInRenderingThread() || IsInRHIThread());
			return SecondaryViewState_RenderThread;
		}
	}
}	 // namespace MicrosoftOpenXR