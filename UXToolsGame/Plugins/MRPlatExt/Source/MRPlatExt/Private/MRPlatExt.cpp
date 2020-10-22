// Copyright (c) Microsoft Corporation.

#include "MRPlatExt.h"

#include "CoreMinimal.h"
#include "HandInteractionPlugin.h"
#include "HolographicWindowAttachmentPlugin.h"
#include "HolographicRemotingPlugin.h"
#include "SpatialAnchorPlugin.h"
#include "Modules/ModuleManager.h"
#include "OpenXrLoaderPlugin.h"
#include "HandMeshPlugin.h"
#include "QRTrackingPlugin.h"
#include "LocatableCamPlugin.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"
#include "SpeechPlugin.h"
#include "SpatialMappingPlugin.h"

#define LOCTEXT_NAMESPACE "FMRPlatExtModule"

namespace MRPlatExt
{
	static class FMRPlatExtModule * g_MRPlatExtModule;

	class FMRPlatExtModule : public IModuleInterface
	{
	public:
		void StartupModule() override
		{
			HandInteractionPlugin.Register();
			SpatialAnchorPlugin.Register();
			HandMeshPlugin.Register();
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
			QRTrackingPlugin.Register();
			LocatableCamPlugin.Register();
			SpeechPlugin.Register();
			SpatialMappingPlugin.Register();
#endif

#if SUPPORTS_REMOTING
			HolographicRemotingPlugin = MakeShared<FHolographicRemotingPlugin>();
			HolographicRemotingPlugin->Register();
#endif

#if PLATFORM_HOLOLENS
			HolographicWindowAttachmentPlugin.Register();
#elif PLATFORM_ANDROID
			OpenXrLoaderPlugin.Register();
#endif

			g_MRPlatExtModule = this;
		}

		void ShutdownModule() override
		{
			g_MRPlatExtModule = nullptr;

			HandInteractionPlugin.Unregister();
			SpatialAnchorPlugin.Unregister();
			HandMeshPlugin.Unregister();
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
			QRTrackingPlugin.Unregister();
			LocatableCamPlugin.Unregister();
			SpeechPlugin.Unregister();
			SpatialMappingPlugin.Unregister();
#endif

#if SUPPORTS_REMOTING
			HolographicRemotingPlugin->Unregister();
#endif

#if PLATFORM_HOLOLENS
			HolographicWindowAttachmentPlugin.Unregister();
#elif PLATFORM_ANDROID
			OpenXrLoaderPlugin.Unregister();
#endif
		}

		FHandInteractionPlugin HandInteractionPlugin;
		FHandMeshPlugin HandMeshPlugin;
		FSpatialAnchorPlugin SpatialAnchorPlugin;
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
		FQRTrackingPlugin QRTrackingPlugin;
		FLocatableCamPlugin LocatableCamPlugin;
		FSpeechPlugin SpeechPlugin;
		FSpatialMappingPlugin SpatialMappingPlugin;
#endif

#if SUPPORTS_REMOTING
		TSharedPtr<FHolographicRemotingPlugin> HolographicRemotingPlugin;
#endif

#if PLATFORM_HOLOLENS
		FHolographicWindowAttachmentPlugin HolographicWindowAttachmentPlugin;
#elif PLATFORM_ANDROID
		FOpenXrLoaderPlugin OpenXrLoaderPlugin;
#endif
	};
}	 // namespace MRPlatExt


bool UMRPlatExtFunctionLibrary::TurnHandMesh(EHandMeshStatus Mode)
{
	return MRPlatExt::g_MRPlatExtModule->HandMeshPlugin.Turn(Mode);
}

bool UMRPlatExtFunctionLibrary::IsQREnabled()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MRPlatExt::g_MRPlatExtModule->QRTrackingPlugin.IsEnabled();
#else
	return false;
#endif
}


FTransform UMRPlatExtFunctionLibrary::GetPVCameraToWorldTransform()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MRPlatExt::g_MRPlatExtModule->LocatableCamPlugin.GetCameraTransform();
#else
	return FTransform::Identity;
#endif
}

bool UMRPlatExtFunctionLibrary::GetPVCameraIntrinsics(FVector2D& focalLength, int& width, int& height, FVector2D& principalPoint, FVector& radialDistortion, FVector2D& tangentialDistortion)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MRPlatExt::g_MRPlatExtModule->LocatableCamPlugin.GetPVCameraIntrinsics(focalLength, width, height, principalPoint, radialDistortion, tangentialDistortion);
#else
	return false;
#endif
}

FVector UMRPlatExtFunctionLibrary::GetWorldSpaceRayFromCameraPoint(FVector2D pixelCoordinate)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MRPlatExt::g_MRPlatExtModule->LocatableCamPlugin.GetWorldSpaceRayFromCameraPoint(pixelCoordinate);
#else
	return FVector::ZeroVector;
#endif
}

bool UMRPlatExtFunctionLibrary::IsSpeechRecognitionAvailable()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return true;
#endif

	return false;
}

void UMRPlatExtFunctionLibrary::AddKeywords(TArray<FKeywordInput> Keywords)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MRPlatExt::g_MRPlatExtModule->SpeechPlugin.AddKeywords(Keywords);
#endif
}

void UMRPlatExtFunctionLibrary::RemoveKeywords(TArray<FString> Keywords)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MRPlatExt::g_MRPlatExtModule->SpeechPlugin.RemoveKeywords(Keywords);
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(MRPlatExt::FMRPlatExtModule, MRPlatExt)
