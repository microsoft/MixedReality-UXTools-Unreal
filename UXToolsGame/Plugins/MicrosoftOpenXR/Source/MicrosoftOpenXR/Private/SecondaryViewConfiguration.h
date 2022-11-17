#pragma once

#include "OpenXRCommon.h"

namespace MicrosoftOpenXR
{
	class FSecondaryViewConfigurationPlugin : public IOpenXRExtensionPlugin
	{
		// This keeps all data structures and memory alive in a safe way for a single projection layer.
		struct SingleProjectionLayer
		{
			SingleProjectionLayer(XrViewConfigurationType InViewConfigType, XrEnvironmentBlendMode InEnvBlendMode, XrSpace InSpace,
				TArray<XrCompositionLayerProjectionView> InViews)
				: ViewConfigType(InViewConfigType), EnvBlendMode(InEnvBlendMode), Space(InSpace), Views(std::move(InViews))
			{
			}

			void Finalize(XrSecondaryViewConfigurationLayerInfoMSFT* SecondaryViewInfo)
			{
				// This struct may be relocated while inside a resizing TArray. Only call this function once the TArray is complete
				// to ensure valid pointers.
				Layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
				Layer.space = Space;
				Layer.viewCount = Views.Num();
				Layer.views = Views.GetData();
				Layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;

				LayerPtrs.Reset();
				LayerPtrs.Add(reinterpret_cast<const XrCompositionLayerBaseHeader*>(&Layer));

				*SecondaryViewInfo = {XR_TYPE_SECONDARY_VIEW_CONFIGURATION_LAYER_INFO_MSFT, nullptr, ViewConfigType, EnvBlendMode,
					(uint32_t) LayerPtrs.Num(),
					LayerPtrs.GetData()};
			}

		private:
			const XrViewConfigurationType ViewConfigType;
			const XrEnvironmentBlendMode EnvBlendMode;
			const XrSpace Space;
			const TArray<XrCompositionLayerProjectionView> Views;

			// The following are set up at
			XrCompositionLayerProjection Layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
			TArray<const XrCompositionLayerBaseHeader*> LayerPtrs;
		};

		struct PiplinedFrameState
		{
			// State of every secondary view configuration that was enabled. Updated on xrWaitFrame.
			TArray<XrSecondaryViewConfigurationStateMSFT> SecondaryViewConfigStates;
			// Latest XrView (Pose and Fov) returned to engine used for rendering.
			TArray<TArray<XrView>> SecondaryViews;

			XrSpace ViewSpace{XR_NULL_HANDLE};
		};

	public:
		void Register();
		void Unregister();

		bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		bool GetOptionalExtensions(TArray<const ANSICHAR*>& OutExtensions) override;

		void PostGetSystem(XrInstance InInstance, XrSystemId InSystem) override;

		const void* OnBeginSession(XrSession InSession, const void* InNext) override;

		void* OnWaitFrame(XrSession InSession, void* InNext) override;
		const void* OnBeginFrame(XrSession InSession, XrTime DisplayTime, const void* InNext) override;

		void GetViewConfigurations(XrSystemId InSystem, TArray<XrViewConfigurationView>& OutViews) override;
		void GetViewLocations(XrSession InSession, XrTime DisplayTime, XrSpace space, TArray<XrView>& OutViews) override;
		const void* OnEndFrame(XrSession InSession, XrTime DisplayTime, const TArray<XrSwapchainSubImage> InColorImages,
			const TArray<XrSwapchainSubImage> InDepthImages, const void* InNext) override;

	private:
		PiplinedFrameState& GetSecondaryViewStateForThread();

	private:
		XrInstance Instance{XR_NULL_HANDLE};
		XrSystemId System{XR_NULL_SYSTEM_ID};

		// View configs that have been enabled on xrBeginSession and their cached properties.
		TArray<XrViewConfigurationType> EnabledViewConfigTypes;
		TArray<XrEnvironmentBlendMode> EnabledViewConfigEnvBlendModes;
		TArray<TArray<XrViewConfigurationView>> EnabledViewConfigurationViews;

		PiplinedFrameState SecondaryViewState_GameThread;
		PiplinedFrameState SecondaryViewState_RenderThread;

		XrSecondaryViewConfigurationSessionBeginInfoMSFT SecondaryViewConfigurationSessionBeginInfo;
		XrSecondaryViewConfigurationFrameStateMSFT SecondaryViewConfigurationFrameState;
		XrSecondaryViewConfigurationFrameEndInfoMSFT SecondaryViewConfigurationEndInfo;

		TArray<SingleProjectionLayer> SecondaryProjectionLayers;
		TArray<XrSecondaryViewConfigurationLayerInfoMSFT> SecondaryViewLayers;
	};
}	 // namespace MicrosoftOpenXR