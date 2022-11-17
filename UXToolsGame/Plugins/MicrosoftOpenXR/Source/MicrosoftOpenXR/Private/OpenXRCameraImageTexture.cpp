// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "OpenXRCameraImageTexture.h"
#include "GlobalShader.h"
#include "RenderUtils.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "ShaderParameterUtils.h"
#include "SceneUtils.h"
#include "MediaShaders.h"
#include "HeadMountedDisplayTypes.h"

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
//-------------------------------------------------------------------------------------------------
// D3D11
//-------------------------------------------------------------------------------------------------

#include "D3D11RHIPrivate.h"
#include "D3D11Util.h"


//-------------------------------------------------------------------------------------------------
// D3D12
//-------------------------------------------------------------------------------------------------

#define GetD3D11CubeFace GetD3D12CubeFace
#define VerifyD3D11Result VerifyD3D12Result
#define GetD3D11TextureFromRHITexture GetD3D12TextureFromRHITexture
#define FRingAllocation FRingAllocation_D3D12
#define GetRenderTargetFormat GetRenderTargetFormat_D3D12
#define ED3D11ShaderOffsetBuffer ED3D12ShaderOffsetBuffer
#define FindShaderResourceDXGIFormat FindShaderResourceDXGIFormat_D3D12
#define FindUnorderedAccessDXGIFormat FindUnorderedAccessDXGIFormat_D3D12
#define FindDepthStencilDXGIFormat FindDepthStencilDXGIFormat_D3D12
#define HasStencilBits HasStencilBits_D3D12
#define FVector4VertexDeclaration FVector4VertexDeclaration_D3D12
#define GLOBAL_CONSTANT_BUFFER_INDEX GLOBAL_CONSTANT_BUFFER_INDEX_D3D12
#define MAX_CONSTANT_BUFFER_SLOTS MAX_CONSTANT_BUFFER_SLOTS_D3D12
#define FD3DGPUProfiler FD3D12GPUProfiler
#define FRangeAllocator FRangeAllocator_D3D12

#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"

#undef GetD3D11CubeFace
#undef VerifyD3D11Result
#undef GetD3D11TextureFromRHITexture
#undef FRingAllocation
#undef GetRenderTargetFormat
#undef ED3D11ShaderOffsetBuffer
#undef FindShaderResourceDXGIFormat
#undef FindUnorderedAccessDXGIFormat
#undef FindDepthStencilDXGIFormat
#undef HasStencilBits
#undef FVector4VertexDeclaration
#undef GLOBAL_CONSTANT_BUFFER_INDEX
#undef MAX_CONSTANT_BUFFER_SLOTS
#undef FD3DGPUProfiler
#undef FRangeAllocator

#include "Windows/AllowWindowsPlatformTypes.h"

THIRD_PARTY_INCLUDES_START
#include <windows.h>
#include <D3D11.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
THIRD_PARTY_INCLUDES_END

#include "Windows/COMPointer.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif


#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

/** Resource class to do all of the setup work on the render thread */
class FOpenXRCameraImageResource :
	public FTextureResource
{
public:
	FOpenXRCameraImageResource(UOpenXRCameraImageTexture* InOwner)
		: LastFrameNumber(0)
		, Owner(InOwner)
	{
	}

	virtual ~FOpenXRCameraImageResource()
	{
	}

	/**
	 * Called when the resource is initialized. This is only called by the rendering thread.
	 */
	virtual void InitRHI() override
	{
		check(IsInRenderingThread());

		FString RHIString = FApp::GetGraphicsRHI();

		bool bIsDx11 = (RHIString == TEXT("DirectX 11"));
		bool bIsDx12 = (RHIString == TEXT("DirectX 12"));

		FSamplerStateInitializerRHI SamplerStateInitializer(SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);
		FTexture2DRHIRef CopyTextureRef;

		bool bDidConvert = false;
		do
		{
			if (!CameraImageHandle)
			{
				break;
			}

			if (bIsDx11)
			{
				FD3D11DynamicRHI* DX11RHI = StaticCast<FD3D11DynamicRHI*>(GDynamicRHI);

				TComPtr<ID3D11Device1> D3D11Device1;
				ensure(SUCCEEDED(DX11RHI->GetDevice()->QueryInterface(IID_PPV_ARGS(&D3D11Device1))));

				TComPtr<ID3D11Texture2D> cameraImageTexture;
				if (FAILED(D3D11Device1->OpenSharedResource1(CameraImageHandle->get(), IID_PPV_ARGS(&cameraImageTexture))))
				{
					UE_LOG(LogHMD, Log, TEXT("ID3D11Device1::OpenSharedResource1 failed in FOpenXRCameraImageResource::InitRHI"));
					break;
				}

				CopyTextureRef = GetID3D11DynamicRHI()->RHICreateTexture2DFromResource(PF_NV12, TexCreate_Dynamic | TexCreate_ShaderResource, FClearValueBinding::None, cameraImageTexture.Get());
			}
			else if (bIsDx12)
			{
				FD3D12DynamicRHI* DX12RHI = StaticCast<FD3D12DynamicRHI*>(GDynamicRHI);

				TComPtr<ID3D12Resource> cameraImageTexture;
				if (FAILED(DX12RHI->GetAdapter().GetD3DDevice()->OpenSharedHandle(CameraImageHandle->get(), IID_PPV_ARGS(&cameraImageTexture))))
				{
					UE_LOG(LogHMD, Log, TEXT("ID3D12Device::OpenSharedHandle failed in FOpenXRCameraImageResource::InitRHI"));
					break;
				}

				CopyTextureRef = GetID3D12DynamicRHI()->RHICreateTexture2DFromResource(PF_NV12, TexCreate_Dynamic, FClearValueBinding::None, cameraImageTexture.Get());
			}
			else
			{
				break;
			}

			if (!CopyTextureRef)
			{
				UE_LOG(LogHMD, Log, TEXT("RHICreateTexture2DFromResource failed in FOpenXRCameraImageResource::InitRHI"));
				break;
			}

			Size = CopyTextureRef->GetSizeXY();

			// Create the render target
			{
				const FRHITextureCreateDesc Desc =
					FRHITextureCreateDesc::Create2D(TEXT("OpenXRDecodedTexture"))
					.SetExtent(Size.X, Size.Y)
					.SetFormat(PF_B8G8R8A8)
					.SetFlags(TexCreate_Dynamic | TexCreate_RenderTargetable);

				DecodedTextureRef = RHICreateTexture(Desc);
			}

			{
				PerformConversion(CopyTextureRef);
				bDidConvert = true;
			}
			// Now that the conversion is done, we can get rid of our refs
			CameraImageHandle = nullptr;
			CopyTextureRef.SafeRelease();
		} while (false);

		// Default to an empty 1x1 texture if we don't have a camera image or failed to convert
		if (!bDidConvert)
		{
			Size.X = Size.Y = 1;

			const FRHITextureCreateDesc Desc =
				FRHITextureCreateDesc::Create2D(TEXT("OpenXRDecodedTextureFallback"))
				.SetExtent(Size.X, Size.Y)
				.SetFormat(PF_B8G8R8A8)
				.SetFlags(TexCreate_ShaderResource);

			DecodedTextureRef = RHICreateTexture(Desc);
		}

		TextureRHI = DecodedTextureRef;
		TextureRHI->SetName(Owner->GetFName());
		RHIBindDebugLabelName(TextureRHI, *Owner->GetName());
		RHIUpdateTextureReference(Owner->TextureReference.TextureReferenceRHI, TextureRHI);
	}

	virtual void ReleaseRHI() override
	{
		RHIUpdateTextureReference(Owner->TextureReference.TextureReferenceRHI, nullptr);
		CameraImageHandle = nullptr;
		DecodedTextureRef.SafeRelease();
		FTextureResource::ReleaseRHI();
	}

	/** Returns the width of the texture in pixels. */
	virtual uint32 GetSizeX() const override
	{
		return Size.X;
	}

	/** Returns the height of the texture in pixels. */
	virtual uint32 GetSizeY() const override
	{
		return Size.Y;
	}

	/** Render thread update of the texture so we don't get 2 updates per frame on the render thread */
	void Init_RenderThread(std::shared_ptr<winrt::handle> handle)
	{
		check(IsInRenderingThread());
		if (LastFrameNumber != GFrameNumber)
		{
			LastFrameNumber = GFrameNumber;
			ReleaseRHI();
			CameraImageHandle = handle;
			InitRHI();
		}
	}

private:

	/** Runs a shader to convert YUV to RGB */
	void PerformConversion(FTexture2DRHIRef CopyTextureRef)
	{
		FRHICommandListImmediate& CommandList = FRHICommandListExecutor::GetImmediateCommandList();
		SCOPED_DRAW_EVENT(CommandList, HoloLensCameraImageConversion);

		FGraphicsPipelineStateInitializer GraphicsPSOInit;
		FRHITexture* RenderTarget = DecodedTextureRef.GetReference();
		CommandList.Transition(FRHITransitionInfo(RenderTarget, ERHIAccess::Unknown, ERHIAccess::RTV));

		FIntPoint OutputDim(RenderTarget->GetSizeXYZ().X, RenderTarget->GetSizeXYZ().Y);

		FRHIRenderPassInfo RPInfo(RenderTarget, ERenderTargetActions::DontLoad_Store);
		CommandList.BeginRenderPass(RPInfo, TEXT("HoloLensCameraImageConversion"));
		{
			CommandList.ApplyCachedRenderTargets(GraphicsPSOInit);
			CommandList.SetViewport(0, 0, 0.0f, OutputDim.X, OutputDim.Y, 1.0f);

			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask<CW_RGBA, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE, CW_NONE>::GetRHI();
			GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;

			// configure media shaders
			auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
			TShaderMapRef<FMediaShadersVS> VertexShader(ShaderMap);

			GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GMediaVertexDeclaration.VertexDeclarationRHI;
			GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();

			// Use the sample format to choose the conversion path
			TShaderMapRef<FNV12ConvertPS> ConvertShader(ShaderMap);
			GraphicsPSOInit.BoundShaderState.PixelShaderRHI = ConvertShader.GetPixelShader();
			SetGraphicsPipelineState(CommandList, GraphicsPSOInit, 0);

			FShaderResourceViewRHIRef Y_SRV = RHICreateShaderResourceView(CopyTextureRef, 0, 1, PF_G8);
			FShaderResourceViewRHIRef UV_SRV = RHICreateShaderResourceView(CopyTextureRef, 0, 1, PF_R8G8);

			ConvertShader->SetParameters(CommandList, CopyTextureRef->GetSizeXY(), Y_SRV, UV_SRV, OutputDim, MediaShaders::YuvToRgbRec601Scaled, MediaShaders::YUVOffset8bits, false);

			// draw full size quad into render target
			FBufferRHIRef VertexBuffer = CreateTempMediaVertexBuffer();
			CommandList.SetStreamSource(0, VertexBuffer, 0);
			// set viewport to RT size
			CommandList.SetViewport(0, 0, 0.0f, OutputDim.X, OutputDim.Y, 1.0f);

			CommandList.DrawPrimitive(0, 2, 1);
		}
		CommandList.EndRenderPass();
		CommandList.Transition(FRHITransitionInfo(RenderTarget, ERHIAccess::RTV, ERHIAccess::SRVGraphics));

	}

	/** The size we get from the incoming camera image */
	FIntPoint Size;

	/** The raw camera image from the HoloLens which we copy to our texture to allow it to be quickly released */
	std::shared_ptr<winrt::handle> CameraImageHandle;
	/** The texture that we actually render with which is populated via a shader that converts nv12 to rgba */
	FTexture2DRHIRef DecodedTextureRef;
	/** The last frame we were updated on */
	uint32 LastFrameNumber;

	const UOpenXRCameraImageTexture* Owner;
};

#endif


UOpenXRCameraImageTexture::UOpenXRCameraImageTexture(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LastUpdateFrame(0)
{
	FString RHIString = FApp::GetGraphicsRHI();
	if (!RHIString.IsEmpty())
	{
		IsDX11 = RHIString == TEXT("DirectX 11");
	}
}

void UOpenXRCameraImageTexture::BeginDestroy()
{
	Super::BeginDestroy();
}

FTextureResource* UOpenXRCameraImageTexture::CreateResource()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return new FOpenXRCameraImageResource(this);
#else
	return nullptr;
#endif
}

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
/** Forces the reconstruction of the texture data and conversion from Nv12 to RGB */
void UOpenXRCameraImageTexture::Init(std::shared_ptr<winrt::handle> handle)
{
	// It's possible that we get more than one queued thread update per game frame
	// Skip any additional frames because it will cause the recursive flush rendering commands ensure
	if (LastUpdateFrame != GFrameCounter)
	{
		LastUpdateFrame = GFrameCounter;
		if (GetResource() != nullptr)
		{
			FOpenXRCameraImageResource* LambdaResource = static_cast<FOpenXRCameraImageResource*>(GetResource());
			ENQUEUE_RENDER_COMMAND(Init_RenderThread)(
				[LambdaResource, handle](FRHICommandListImmediate&)
			{
				LambdaResource->Init_RenderThread(handle);
			});
		}
		else
		{
			// This should end up only being called once, the first time we get a texture update
			UpdateResource();
		}
	}
}
#endif