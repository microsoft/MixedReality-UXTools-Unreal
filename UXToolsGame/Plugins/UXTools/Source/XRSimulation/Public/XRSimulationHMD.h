// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "HeadMountedDisplayBase.h"
#include "SceneViewExtension.h"
#include "XRRenderTargetManager.h"
#include "XRSimulationActor.h"
#include "XRTrackingSystemBase.h"

class APlayerController;
class FSceneView;
class FSceneViewFamily;
class UCanvas;
class AGameModeBase;
class AController;
class APlayerController;
class UInputComponent;
struct FXRSimulationState;

/**
 * Simple Head Mounted Display
 */
class XRSIMULATION_API FXRSimulationHMD
	: public FHeadMountedDisplayBase
	, public FXRRenderTargetManager
	, public FSceneViewExtensionBase
{
public:
	static const FName SystemName;

	/** IXRTrackingSystem interface */
	virtual FName GetSystemName() const override { return SystemName; }

	int32 GetXRSystemFlags() const { return EXRSystemFlags::IsHeadMounted; }

	virtual bool EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type = EXRTrackedDeviceType::Any) override;

	virtual void SetInterpupillaryDistance(float NewInterpupillaryDistance) override;
	virtual float GetInterpupillaryDistance() const override;

	virtual void ResetOrientationAndPosition(float Yaw = 0.f) override;
	virtual void ResetOrientation(float Yaw = 0.f) override;
	virtual void ResetPosition() override;

	virtual bool GetCurrentPose(int32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition) override;
	virtual void SetBaseRotation(const FRotator& BaseRot) override;
	virtual FRotator GetBaseRotation() const override;

	virtual void SetBaseOrientation(const FQuat& BaseOrient) override;
	virtual FQuat GetBaseOrientation() const override;

	virtual bool DoesSupportPositionalTracking() const override;

	virtual class IHeadMountedDisplay* GetHMDDevice() override { return this; }

	virtual class TSharedPtr<class IStereoRendering, ESPMode::ThreadSafe> GetStereoRenderingDevice() override
	{
		return SharedThis(this);
	}

	virtual void
	OnBeginPlay(FWorldContext& InWorldContext) override;

	virtual void OnEndPlay(FWorldContext& InWorldContext) override;

	virtual void OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily) override;
	// Spectator screen Hooks.
	virtual FIntRect GetFullFlatEyeRect_RenderThread(FTexture2DRHIRef EyeTexture) const override;
	// Helper to copy one render target into another for spectator screen display
	virtual void CopyTexture_RenderThread(
		FRHICommandListImmediate& RHICmdList, FRHITexture2D* SrcTexture, FIntRect SrcRect, FRHITexture2D* DstTexture, FIntRect DstRect,
		bool bClearBlack, bool bNoAlpha) const override;

	virtual void GetMotionControllerData(
		UObject* WorldContext, const EControllerHand Hand, FXRMotionControllerData& MotionControllerData) override;

protected:
	/** FXRTrackingSystemBase protected interface */
	virtual float GetWorldToMetersScale() const override;

public:
	/** IHeadMountedDisplay interface */
	virtual bool IsHMDConnected() override;
	virtual bool IsHMDEnabled() const override;
	virtual void EnableHMD(bool bEnable = true) override;
	virtual bool GetHMDMonitorInfo(MonitorInfo&) override;
	virtual void GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const override;
	virtual bool IsChromaAbCorrectionEnabled() const override;
	virtual FIntPoint GetIdealRenderTargetSize() const override;
	virtual bool OnStartGameFrame(FWorldContext& WorldContext) override;

	/** IStereoRendering interface */
	virtual bool IsStereoEnabled() const override;
	virtual bool EnableStereo(bool bEnable = true) override;
	virtual void AdjustViewRect(EStereoscopicPass StereoPass, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const override;
	virtual EStereoscopicPass GetViewPassForIndex(bool bStereoRequested, uint32 ViewIndex) const override;
	virtual uint32 GetViewIndexForPass(EStereoscopicPass StereoPassType) const override;
	virtual int32 GetDesiredNumberOfViews(bool bStereoRequested) const override;
	virtual FMatrix GetStereoProjectionMatrix(const enum EStereoscopicPass StereoPassType) const override;
	virtual void GetEyeRenderParams_RenderThread(
		const struct FRenderingCompositePassContext& Context, FVector2D& EyeToSrcUVScaleValue,
		FVector2D& EyeToSrcUVOffsetValue) const override;
	virtual IStereoRenderTargetManager* GetRenderTargetManager() override;
	virtual void RenderTexture_RenderThread(
		class FRHICommandListImmediate& RHICmdList, class FRHITexture2D* BackBuffer, class FRHITexture2D* SrcTexture,
		FVector2D WindowSize) const override;

	/** IStereoRenderTargetManager */
	virtual bool ShouldUseSeparateRenderTarget() const override;

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;

protected:
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

public:
	/** Constructor */
	FXRSimulationHMD(const FAutoRegister&);

	/** Destructor */
	virtual ~FXRSimulationHMD();

	/** @return	True if the HMD was initialized OK */
	bool IsInitialized() const;

	AXRSimulationActor* GetSimulationActor() const;

private:
	/** Ensures that viewport is set up for stereographic rendering.
	 * Disables stereo rendering if the viewport does not support it.
	 */
	void SetupStereoViewport();
	/** Reset viewport to non-stereographic rendering settings. */
	void ShutdownStereoViewport();

	void OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer);
	void OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting);

	AXRSimulationActor* GetOrCreateInputSimActor(APlayerController* PlayerController);
	void DestroyInputSimActor();

private:
	IRendererModule* RendererModule = nullptr;

	FDelegateHandle OnPostLoginDelegateHandle;
	FDelegateHandle OnLogoutDelegateHandle;

	bool bIsInitialized = false;
	bool bHMDEnabled = true;
	bool bStereoEnabled = false;

	/** Actor that handles user input to simulate XR. */
	TWeakObjectPtr<AXRSimulationActor> SimulationActorWeak;

	TSharedPtr<FXRSimulationState> SimulationState;

	float WorldToMeters = 100.0f;
	int32 RecommendedImageRectWidth = 2048;
	int32 RecommendedImageRectHeight = 1080;
};
