// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "XRSimulationHMD.h"

#include "ClearQuad.h"
#include "CommonRenderResources.h"
#include "EngineGlobals.h"
#include "RenderUtils.h"
#include "ScreenRendering.h"
#include "XRSimulationHeadMovementComponent.h"
#include "XRSimulationRuntimeSettings.h"
#include "XRSimulationState.h"

#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Modules/ModuleManager.h"
#include "Slate/SceneViewport.h"

#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogXRSimulationHMD, Log, All);

namespace
{
	/** Helper function for acquiring the appropriate FSceneViewport */
	FSceneViewport* FindSceneViewport()
	{
		if (!GIsEditor)
		{
			UGameEngine* GameEngine = Cast<UGameEngine>(GEngine);
			return GameEngine->SceneViewport.Get();
		}
#if WITH_EDITOR
		else
		{
			UEditorEngine* EditorEngine = CastChecked<UEditorEngine>(GEngine);
			FSceneViewport* PIEViewport = (FSceneViewport*)EditorEngine->GetPIEViewport();
			if (PIEViewport != nullptr && PIEViewport->IsStereoRenderingAllowed())
			{
				// PIE is setup for stereo rendering
				return PIEViewport;
			}
			else
			{
				// Check to see if the active editor viewport is drawing in stereo mode
				// @todo vreditor: Should work with even non-active viewport!
				FSceneViewport* EditorViewport = (FSceneViewport*)EditorEngine->GetActiveViewport();
				if (EditorViewport != nullptr && EditorViewport->IsStereoRenderingAllowed())
				{
					return EditorViewport;
				}
			}
		}
#endif
		return nullptr;
	}
} // namespace

const FName FXRSimulationHMD::SystemName = TEXT("XRSimulation");

float FXRSimulationHMD::GetWorldToMetersScale() const
{
	return WorldToMeters;
}

//---------------------------------------------------
// SimpleHMD IHeadMountedDisplay Implementation
//---------------------------------------------------

bool FXRSimulationHMD::IsHMDConnected()
{
	const UXRSimulationRuntimeSettings* Settings = UXRSimulationRuntimeSettings::Get();
	if (Settings)
	{
		return Settings->bEnableSimulation;
	}
	return false;
}

bool FXRSimulationHMD::IsHMDEnabled() const
{
	return bHMDEnabled;
}

void FXRSimulationHMD::EnableHMD(bool bEnable)
{
	bHMDEnabled = bEnable;
	if (!bHMDEnabled)
	{
		EnableStereo(false);
	}
}

bool FXRSimulationHMD::GetHMDMonitorInfo(MonitorInfo& MonitorDesc)
{
	MonitorDesc.MonitorName = "";
	MonitorDesc.MonitorId = 0;
	MonitorDesc.DesktopX = MonitorDesc.DesktopY = MonitorDesc.ResolutionX = MonitorDesc.ResolutionY = 0;
	return false;
}

void FXRSimulationHMD::GetFieldOfView(float& OutHFOVInDegrees, float& OutVFOVInDegrees) const
{
	OutHFOVInDegrees = 0.0f;
	OutVFOVInDegrees = 0.0f;
}

bool FXRSimulationHMD::EnumerateTrackedDevices(TArray<int32>& OutDevices, EXRTrackedDeviceType Type)
{
	if (Type == EXRTrackedDeviceType::Any || Type == EXRTrackedDeviceType::HeadMountedDisplay)
	{
		OutDevices.Add(IXRTrackingSystem::HMDDeviceId);
		return true;
	}
	return false;
}

void FXRSimulationHMD::SetInterpupillaryDistance(float NewInterpupillaryDistance)
{
}

float FXRSimulationHMD::GetInterpupillaryDistance() const
{
	return 0.064f;
}

bool FXRSimulationHMD::GetCurrentPose(int32 DeviceId, FQuat& CurrentOrientation, FVector& CurrentPosition)
{
	if (DeviceId == IXRTrackingSystem::HMDDeviceId)
	{
		if (AXRSimulationActor* InputSimActor = SimulationActorWeak.Get())
		{
			InputSimActor->GetHeadPose(CurrentOrientation, CurrentPosition);
			return true;
		}
	}
	return false;
}

bool FXRSimulationHMD::IsChromaAbCorrectionEnabled() const
{
	return false;
}

void FXRSimulationHMD::ResetOrientationAndPosition(float yaw)
{
	ResetOrientation(yaw);
	ResetPosition();
}

void FXRSimulationHMD::ResetOrientation(float Yaw)
{
}

void FXRSimulationHMD::ResetPosition()
{
}

void FXRSimulationHMD::SetBaseRotation(const FRotator& BaseRot)
{
}

FRotator FXRSimulationHMD::GetBaseRotation() const
{
	return FRotator::ZeroRotator;
}

void FXRSimulationHMD::SetBaseOrientation(const FQuat& BaseOrient)
{
}

FQuat FXRSimulationHMD::GetBaseOrientation() const
{
	return FQuat::Identity;
}

bool FXRSimulationHMD::DoesSupportPositionalTracking() const
{
	return true;
}

void FXRSimulationHMD::OnBeginPlay(FWorldContext& InWorldContext)
{
	// BeginPlay is also called during map load, but then not followed up by EndPlay.
	// Make sure to only run this once, to prevent leftover delegate registrations.
	if (bIsInitialized)
	{
		return;
	}
	bIsInitialized = true;

	// Only use input simulation in VR preview mode.
	// Cannot test for stereoscopic viewport because it is only created later
	// after BeginPlay and player login.
	bool bIsVRPreview = false;
#if WITH_EDITOR
	if (GIsEditor)
	{
		UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
		if (EdEngine->GetPlayInEditorSessionInfo().IsSet())
		{
			bIsVRPreview = EdEngine->GetPlayInEditorSessionInfo()->OriginalRequestParams.SessionPreviewTypeOverride ==
						   EPlaySessionPreviewType::VRPreview;
		}
	}
#endif

	if (bHMDEnabled && bIsVRPreview)
	{
		AXRSimulationActor::RegisterInputMappings();

		SimulationState = MakeShareable(new FXRSimulationState());

		OnPostLoginDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddRaw(this, &FXRSimulationHMD::OnGameModePostLogin);
		OnLogoutDelegateHandle = FGameModeEvents::GameModeLogoutEvent.AddRaw(this, &FXRSimulationHMD::OnGameModeLogout);
	}
}

void FXRSimulationHMD::OnEndPlay(FWorldContext& InWorldContext)
{
	FGameModeEvents::GameModePostLoginEvent.Remove(OnPostLoginDelegateHandle);
	FGameModeEvents::GameModeLogoutEvent.Remove(OnLogoutDelegateHandle);

	AXRSimulationActor::UnregisterInputMappings();

	bIsInitialized = false;
}

void FXRSimulationHMD::OnBeginRendering_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	if (SpectatorScreenController)
	{
		SpectatorScreenController->UpdateSpectatorScreenMode_RenderThread();
	}
}

FIntRect FXRSimulationHMD::GetFullFlatEyeRect_RenderThread(FTexture2DRHIRef EyeTexture) const
{
	FVector2D SrcNormRectMin(0.0f, 0.0f);
	FVector2D SrcNormRectMax(1.0f, 1.0f);

	return FIntRect(
		EyeTexture->GetSizeX() * SrcNormRectMin.X, EyeTexture->GetSizeY() * SrcNormRectMin.Y, EyeTexture->GetSizeX() * SrcNormRectMax.X,
		EyeTexture->GetSizeY() * SrcNormRectMax.Y);
}

void FXRSimulationHMD::CopyTexture_RenderThread(
	FRHICommandListImmediate& RHICmdList, FRHITexture2D* SrcTexture, FIntRect SrcRect, FRHITexture2D* DstTexture, FIntRect DstRect,
	bool bClearBlack, bool bNoAlpha) const
{
	check(IsInRenderingThread());

	const uint32 ViewportWidth = DstRect.Width();
	const uint32 ViewportHeight = DstRect.Height();
	const FIntPoint TargetSize(ViewportWidth, ViewportHeight);

	const float SrcTextureWidth = SrcTexture->GetSizeX();
	const float SrcTextureHeight = SrcTexture->GetSizeY();
	float U = 0.f, V = 0.f, USize = 1.f, VSize = 1.f;
	if (!SrcRect.IsEmpty())
	{
		U = SrcRect.Min.X / SrcTextureWidth;
		V = SrcRect.Min.Y / SrcTextureHeight;
		USize = SrcRect.Width() / SrcTextureWidth;
		VSize = SrcRect.Height() / SrcTextureHeight;
	}

	FRHITexture* ColorRT = DstTexture->GetTexture2D();
	FRHIRenderPassInfo RenderPassInfo(ColorRT, ERenderTargetActions::DontLoad_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("XRSimulationHMD_CopyTexture"));
	{
		if (bClearBlack)
		{
			const FIntRect ClearRect(0, 0, DstTexture->GetSizeX(), DstTexture->GetSizeY());
			RHICmdList.SetViewport(ClearRect.Min.X, ClearRect.Min.Y, 0, ClearRect.Max.X, ClearRect.Max.Y, 1.0f);
			DrawClearQuad(RHICmdList, FLinearColor::Black);
		}

		RHICmdList.SetViewport(DstRect.Min.X, DstRect.Min.Y, 0, DstRect.Max.X, DstRect.Max.Y, 1.0f);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
		GraphicsPSOInit.BlendState =
			bNoAlpha
				? TStaticBlendState<>::GetRHI()
				: TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
		GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
		GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		const auto FeatureLevel = GMaxRHIFeatureLevel;
		auto ShaderMap = GetGlobalShaderMap(FeatureLevel);

		TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
		TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();

		SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

		RHICmdList.Transition(FRHITransitionInfo(SrcTexture, ERHIAccess::Unknown, ERHIAccess::SRVMask));

		const bool bSameSize = DstRect.Size() == SrcRect.Size();
		if (bSameSize)
		{
			PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), SrcTexture);
		}
		else
		{
			PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SrcTexture);
		}

		RendererModule->DrawRectangle(
			RHICmdList, 0, 0, ViewportWidth, ViewportHeight, U, V, USize, VSize, TargetSize, FIntPoint(1, 1), VertexShader, EDRF_Default);
	}
	RHICmdList.EndRenderPass();
}

void FXRSimulationHMD::GetMotionControllerData(
	UObject* WorldContext, const EControllerHand Hand, FXRMotionControllerData& MotionControllerData)
{
	MotionControllerData.DeviceName = GetSystemName();
	MotionControllerData.ApplicationInstanceID = FApp::GetInstanceId();

	// Default values
	MotionControllerData.DeviceVisualType = EXRVisualType::Controller;
	MotionControllerData.TrackingStatus = ETrackingStatus::NotTracked;
	MotionControllerData.HandIndex = Hand;

	if (AXRSimulationActor* InputSimActor = SimulationActorWeak.Get())
	{
		InputSimActor->GetHandData(Hand, MotionControllerData);
	}

	// TODO: this is reportedly a wmr specific convenience function for rapid prototyping.
	MotionControllerData.bIsGrasped = false;
}

FIntPoint FXRSimulationHMD::GetIdealRenderTargetSize() const
{
	FIntPoint Size(EForceInit::ForceInitToZero);
	Size.X = FMath::Max(Size.X, RecommendedImageRectWidth);
	Size.Y = FMath::Max(Size.Y, RecommendedImageRectHeight);

	// We always prefer the nearest multiple of 4 for our buffer sizes. Make sure we round up here,
	// so we're consistent with the rest of the engine in creating our buffers.
	QuantizeSceneBufferSize(Size, Size);

	return Size;
}

bool FXRSimulationHMD::OnStartGameFrame(FWorldContext& WorldContext)
{
	const AWorldSettings* const WorldSettings = WorldContext.World() ? WorldContext.World()->GetWorldSettings() : nullptr;
	if (WorldSettings)
	{
		WorldToMeters = WorldSettings->WorldToMeters;
	}

	// Only refresh this based on the game world.  When remoting there is also an editor world, which we do not want to have affect the
	// transform.
	if (WorldContext.World()->IsGameWorld())
	{
		RefreshTrackingToWorldTransform(WorldContext);
		if (AXRSimulationActor* SimActor = SimulationActorWeak.Get())
		{
			SimActor->SetTrackingToWorldTransform(GetTrackingToWorldTransform());
		}
	}

	GetARCompositionComponent()->StartARGameFrame(WorldContext);

	return true;
}

bool FXRSimulationHMD::IsStereoEnabled() const
{
	return bHMDEnabled && bStereoEnabled;
}

bool FXRSimulationHMD::EnableStereo(bool bEnable)
{
	bStereoEnabled = bEnable;
	if (bStereoEnabled)
	{
		SetupStereoViewport();
	}
	else
	{
		ShutdownStereoViewport();
	}

	return bStereoEnabled;
}

void FXRSimulationHMD::AdjustViewRect(int32 ViewIndex, int32& X, int32& Y, uint32& SizeX, uint32& SizeY) const
{
	FIntPoint ViewRectMin(EForceInit::ForceInitToZero);
	for (int32 i = 0; i < ViewIndex; ++i)
	{
		ViewRectMin.X += RecommendedImageRectWidth;
	}
	QuantizeSceneBufferSize(ViewRectMin, ViewRectMin);

	X = ViewRectMin.X;
	Y = ViewRectMin.Y;
	SizeX = RecommendedImageRectWidth;
	SizeY = RecommendedImageRectHeight;
}

EStereoscopicPass FXRSimulationHMD::GetViewPassForIndex(bool bStereoRequested, int32 ViewIndex) const
{
	if (!bStereoRequested)
		return EStereoscopicPass::eSSP_FULL;

	return ViewIndex == EStereoscopicEye::eSSE_LEFT_EYE ? EStereoscopicPass::eSSP_PRIMARY : EStereoscopicPass::eSSP_SECONDARY;
}

int32 FXRSimulationHMD::GetDesiredNumberOfViews(bool bStereoRequested) const
{
	return bStereoRequested ? 2 : 1;
}

FMatrix FXRSimulationHMD::GetStereoProjectionMatrix(const int32 ViewIndex) const
{
	const float ZNear = GNearClippingPlane;
	const float AngleUp = FMath::DegreesToRadians(30.0f);
	const float AngleDown = FMath::DegreesToRadians(-30.0f);
	const float AngleLeft = FMath::DegreesToRadians(-40.0f);
	const float AngleRight = FMath::DegreesToRadians(40.0f);

	const float TanAngleUp = tan(AngleUp);
	const float TanAngleDown = tan(AngleDown);
	const float TanAngleLeft = tan(AngleLeft);
	const float TanAngleRight = tan(AngleRight);

	float SumRL = (TanAngleRight + TanAngleLeft);
	float SumTB = (TanAngleUp + TanAngleDown);
	float InvRL = (1.0f / (TanAngleRight - TanAngleLeft));
	float InvTB = (1.0f / (TanAngleUp - TanAngleDown));

	FMatrix Mat = FMatrix(
		FPlane((2.0f * InvRL), 0.0f, 0.0f, 0.0f), FPlane(0.0f, (2.0f * InvTB), 0.0f, 0.0f),
		FPlane((SumRL * -InvRL), (SumTB * -InvTB), 0.0f, 1.0f), FPlane(0.0f, 0.0f, ZNear, 0.0f));

	return Mat;
}

void FXRSimulationHMD::GetEyeRenderParams_RenderThread(
	const FHeadMountedDisplayPassContext& Context, FVector2D& EyeToSrcUVScaleValue, FVector2D& EyeToSrcUVOffsetValue) const
{
	EyeToSrcUVOffsetValue = FVector2D::ZeroVector;
	EyeToSrcUVScaleValue = FVector2D(1.0f, 1.0f);
}

IStereoRenderTargetManager* FXRSimulationHMD::GetRenderTargetManager()
{
	return this;
}

void FXRSimulationHMD::RenderTexture_RenderThread(
	class FRHICommandListImmediate& RHICmdList, class FRHITexture2D* BackBuffer, class FRHITexture2D* SrcTexture,
	FVector2D WindowSize) const
{
	if (SpectatorScreenController)
	{
		SpectatorScreenController->RenderSpectatorScreen_RenderThread(RHICmdList, BackBuffer, SrcTexture, WindowSize);
	}
}

bool FXRSimulationHMD::ShouldUseSeparateRenderTarget() const
{
	return IsStereoEnabled();
}

void FXRSimulationHMD::SetupViewFamily(FSceneViewFamily& InViewFamily)
{
	InViewFamily.EngineShowFlags.MotionBlur = 0;
	InViewFamily.EngineShowFlags.HMDDistortion = true;
	InViewFamily.EngineShowFlags.StereoRendering = false; // IsStereoEnabled();
}

void FXRSimulationHMD::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	if (AXRSimulationActor* InputSimActor = SimulationActorWeak.Get())
	{
		InputSimActor->GetHeadPose(InView.BaseHmdOrientation, InView.BaseHmdLocation);
	}
}

void FXRSimulationHMD::PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView)
{
	check(IsInRenderingThread());
}

void FXRSimulationHMD::PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());
}

bool FXRSimulationHMD::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return GEngine && GEngine->IsStereoscopic3D(Context.Viewport);
}

FXRSimulationHMD::FXRSimulationHMD(const FAutoRegister& AutoRegister)
	: FHeadMountedDisplayBase(nullptr)
	, FSceneViewExtensionBase(AutoRegister)
{
	static const FName RendererModuleName("Renderer");
	RendererModule = FModuleManager::GetModulePtr<IRendererModule>(RendererModuleName);

	SpectatorScreenController = MakeUnique<FDefaultSpectatorScreenController>(this);
}

FXRSimulationHMD::~FXRSimulationHMD()
{
}

bool FXRSimulationHMD::IsInitialized() const
{
	return true;
}

AXRSimulationActor* FXRSimulationHMD::GetSimulationActor() const
{
	return SimulationActorWeak.Get();
}

void FXRSimulationHMD::SetupStereoViewport()
{
	FSceneViewport* Viewport = FindSceneViewport();

	if (Viewport)
	{
		TSharedPtr<SWindow> Window = Viewport->FindWindow();
		{
			if (Viewport->IsStereoRenderingAllowed())
			{
				Viewport->SetViewportSize(RecommendedImageRectWidth, RecommendedImageRectHeight);
				Window->SetViewportSizeDrivenByWindow(false);
			}
			else
			{
				FVector2D size = Viewport->FindWindow()->GetSizeInScreen();
				Viewport->SetViewportSize(size.X, size.Y);
				Window->SetViewportSizeDrivenByWindow(true);

				bStereoEnabled = false;
			}
		}
	}
}

void FXRSimulationHMD::ShutdownStereoViewport()
{
	// Reset the viewport to no longer match the HMD display
	FSceneViewport* Viewport = FindSceneViewport();
	if (Viewport)
	{
		TSharedPtr<SWindow> Window = Viewport->FindWindow();
		if (Window.IsValid())
		{
			Window->SetViewportSizeDrivenByWindow(true);

			bStereoEnabled = false;
		}
	}
}

void FXRSimulationHMD::OnGameModePostLogin(AGameModeBase* GameMode, APlayerController* NewPlayer)
{
	if (NewPlayer->IsLocalPlayerController())
	{
		AXRSimulationActor* SimActor = GetOrCreateInputSimActor(NewPlayer);
	}
}

void FXRSimulationHMD::OnGameModeLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (Exiting->IsLocalPlayerController())
	{
		DestroyInputSimActor();
	}
}

AXRSimulationActor* FXRSimulationHMD::GetOrCreateInputSimActor(APlayerController* PlayerController)
{
	// Only create one actor
	if (SimulationActorWeak.IsValid())
	{
		return SimulationActorWeak.Get();
	}

	FActorSpawnParameters p;
	p.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	p.bDeferConstruction = true;

	UWorld* World = PlayerController->GetWorld();
	AXRSimulationActor* InputSimActor = World->SpawnActor<AXRSimulationActor>(p);
	SimulationActorWeak = InputSimActor;

	InputSimActor->SetSimulationState(SimulationState);

	// Explicitly enable input: The simulation actor may be created after loading a map,
	// in which case auto-enabling input does not work.
	InputSimActor->EnableInput(PlayerController);

	UGameplayStatics::FinishSpawningActor(InputSimActor, FTransform::Identity);

	return InputSimActor;
}

void FXRSimulationHMD::DestroyInputSimActor()
{
	if (AActor* InputSimActor = SimulationActorWeak.Get())
	{
		InputSimActor->Destroy();
	}
	SimulationActorWeak.Reset();
}
