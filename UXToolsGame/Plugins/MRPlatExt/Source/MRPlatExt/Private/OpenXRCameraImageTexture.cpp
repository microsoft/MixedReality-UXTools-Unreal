#include "OpenXRCameraImageTexture.h"

#include "GlobalShader.h"
#include "RenderUtils.h"
#include "RHIStaticStates.h"
#include "PipelineStateCache.h"
#include "ShaderParameterUtils.h"
#include "SceneUtils.h"
#include "MediaShaders.h"
#include "HeadMountedDisplayTypes.h"

#include "Windows/AllowWindowsPlatformTypes.h"

THIRD_PARTY_INCLUDES_START
#include <windows.h>
#include <D3D11.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
THIRD_PARTY_INCLUDES_END

#include "Windows/COMPointer.h"
#include "Windows/HideWindowsPlatformTypes.h"



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

		FSamplerStateInitializerRHI SamplerStateInitializer(SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);

		bool bDidConvert = false;
		if (CameraImageHandle)
		{
			// Open the shared texture from the HoloLens camera on Unreal's d3d device.
			ID3D11Device* D3D11Device = static_cast<ID3D11Device*>(GDynamicRHI->RHIGetNativeDevice());
			TComPtr<ID3D11DeviceContext> D3D11DeviceContext = nullptr;
			D3D11Device->GetImmediateContext(&D3D11DeviceContext);
			if (D3D11DeviceContext == nullptr)
			{
				CameraImageHandle = nullptr;

				return;
			}

			TComPtr<ID3D11Texture2D> cameraImageTexture;
			TComPtr<IDXGIResource1> cameraImageResource(NULL);
			if (FAILED(((ID3D11Device1*)D3D11Device)->OpenSharedResource1(CameraImageHandle->get(), __uuidof(IDXGIResource1), (void**)&cameraImageResource)))
			{
				UE_LOG(LogHMD, Log, TEXT("ID3D11Device1::OpenSharedResource1 failed in FOpenXRCameraImageResource::InitRHI"));
				return;
			}
			if (FAILED(cameraImageResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)(&cameraImageTexture))))
			{
				UE_LOG(LogHMD, Log, TEXT("IDXGIResource1::QueryInterface failed in FOpenXRCameraImageResource::InitRHI"));
				return;
			}

			D3D11_TEXTURE2D_DESC Desc;
			cameraImageTexture->GetDesc(&Desc);

			Size.X = Desc.Width;
			Size.Y = Desc.Height;

			// Create the copy target
			{
				FRHIResourceCreateInfo CreateInfo;
				CopyTextureRef = RHICreateTexture2D(Size.X, Size.Y, PF_NV12, 1, 1, TexCreate_Dynamic | TexCreate_ShaderResource, CreateInfo);
			}
			// Create the render target
			{
				FRHIResourceCreateInfo CreateInfo;
				TRefCountPtr<FRHITexture2D> DummyTexture2DRHI;
				// Create our render target that we'll convert to
				RHICreateTargetableShaderResource2D(Size.X, Size.Y, PF_B8G8R8A8, 1, TexCreate_Dynamic, TexCreate_RenderTargetable, false, CreateInfo, DecodedTextureRef, DummyTexture2DRHI);
			}

			if (PerformCopy(cameraImageTexture, D3D11DeviceContext))
			{
				PerformConversion();
				bDidConvert = true;
			}
			// Now that the conversion is done, we can get rid of our refs
			CameraImageHandle = nullptr;
			CopyTextureRef.SafeRelease();
		}

		// Default to an empty 1x1 texture if we don't have a camera image or failed to convert
		if (!bDidConvert)
		{
			FRHIResourceCreateInfo CreateInfo;
			Size.X = Size.Y = 1;
			DecodedTextureRef = RHICreateTexture2D(Size.X, Size.Y, PF_B8G8R8A8, 1, 1, TexCreate_ShaderResource, CreateInfo);
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
		CopyTextureRef.SafeRelease();
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
	/** Copy CameraImage to our CopyTextureRef using the GPU */
	bool PerformCopy(const TComPtr<ID3D11Texture2D>& texture, const TComPtr<ID3D11DeviceContext>& context)
	{
		// These must already be prepped
		if (texture == nullptr 
			|| context == nullptr
			|| !CopyTextureRef.IsValid())
		{
			return false;
		}
		// Get the underlying interface for the texture we are copying to
		TComPtr<ID3D11Resource> CopyTexture = reinterpret_cast<ID3D11Resource*>(CopyTextureRef->GetNativeResource());
		if (CopyTexture == nullptr)
		{
			return false;
		}

		context->CopyResource(CopyTexture.Get(), texture);

		return true;
	}

	/** Runs a shader to convert YUV to RGB */
	void PerformConversion()
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
			SetGraphicsPipelineState(CommandList, GraphicsPSOInit);

			FShaderResourceViewRHIRef Y_SRV = RHICreateShaderResourceView(CopyTextureRef, 0, 1, PF_G8);
			FShaderResourceViewRHIRef UV_SRV = RHICreateShaderResourceView(CopyTextureRef, 0, 1, PF_R8G8);

			ConvertShader->SetParameters(CommandList, CopyTextureRef->GetSizeXY(), Y_SRV, UV_SRV, OutputDim, MediaShaders::YuvToSrgbDefault, MediaShaders::YUVOffset8bits, false);

			// draw full size quad into render target
			FVertexBufferRHIRef VertexBuffer = CreateTempMediaVertexBuffer();
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
	/** The nv12 texture that we copy into so we don't block the camera from being able to send frames */
	FTexture2DRHIRef CopyTextureRef;
	/** The texture that we actually render with which is populated via a shader that converts nv12 to rgba */
	FTexture2DRHIRef DecodedTextureRef;
	/** The last frame we were updated on */
	uint32 LastFrameNumber;

	const UOpenXRCameraImageTexture* Owner;
};



UOpenXRCameraImageTexture::UOpenXRCameraImageTexture(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LastUpdateFrame(0)
{
}

void UOpenXRCameraImageTexture::BeginDestroy()
{
	Super::BeginDestroy();
}

FTextureResource* UOpenXRCameraImageTexture::CreateResource()
{
	return new FOpenXRCameraImageResource(this);
}

/** Forces the reconstruction of the texture data and conversion from Nv12 to RGB */
void UOpenXRCameraImageTexture::Init(std::shared_ptr<winrt::handle> handle)
{
	// It's possible that we get more than one queued thread update per game frame
	// Skip any additional frames because it will cause the recursive flush rendering commands ensure
	if (LastUpdateFrame != GFrameCounter)
	{
		LastUpdateFrame = GFrameCounter;
		if (Resource != nullptr)
		{
			FOpenXRCameraImageResource* LambdaResource = static_cast<FOpenXRCameraImageResource*>(Resource);
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
