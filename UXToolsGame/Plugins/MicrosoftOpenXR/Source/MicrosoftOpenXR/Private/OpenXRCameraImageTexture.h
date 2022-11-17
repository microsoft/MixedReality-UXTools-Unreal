// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ARTextures.h"
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include <memory>
#include <winrt/base.h>
#endif


#include "OpenXRCameraImageTexture.generated.h"

/**
 * Provides access to the camera's image data as a texture
 */
UCLASS(NotBlueprintType)
class UOpenXRCameraImageTexture :
	public UARTextureCameraImage
{
	GENERATED_UCLASS_BODY()

public:
	// UTexture interface implementation
	virtual void BeginDestroy() override;
	virtual FTextureResource* CreateResource() override;
	virtual EMaterialValueType GetMaterialType() const override { return MCT_Texture2D; }
	// End UTexture interface

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	/** Forces the reconstruction of the texture data and conversion from Nv12 to RGB */
	virtual void Init(std::shared_ptr<winrt::handle> handle);
#endif

	friend class FOpenXRCameraImageResource;

private:
	/** Used to prevent two updates of the texture in the same game frame */
	uint64 LastUpdateFrame;

	bool IsDX11 = false;
};

