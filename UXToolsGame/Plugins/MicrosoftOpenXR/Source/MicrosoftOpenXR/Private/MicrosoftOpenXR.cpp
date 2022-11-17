// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "MicrosoftOpenXR.h"

#include "AzureObjectAnchorsPlugin.h"
#include "CoreMinimal.h"
#include "HandMeshPlugin.h"
#include "HolographicRemotingPlugin.h"
#include "HolographicWindowAttachmentPlugin.h"
#include "Interfaces/IPluginManager.h"
#include "LocatableCamPlugin.h"
#include "Modules/ModuleManager.h"
#include "QRTrackingPlugin.h"
#include "SceneUnderstandingPlugin.h"
#include "SecondaryViewConfiguration.h"
#include "ShaderCore.h"
#include "SpatialAnchorPlugin.h"
#include "SpatialMappingPlugin.h"
#include "SpeechPlugin.h"

DEFINE_LOG_CATEGORY(LogAOA)

#define LOCTEXT_NAMESPACE "FMicrosoftOpenXRModule"

namespace MicrosoftOpenXR
{
	static class FMicrosoftOpenXRModule* g_MicrosoftOpenXRModule;

	class FMicrosoftOpenXRModule : public IModuleInterface
	{
	public:
		void StartupModule() override
		{
			SpatialAnchorPlugin.Register();
			HandMeshPlugin.Register();
			SecondaryViewConfigurationPlugin.Register();
			SceneUnderstandingPlugin.Register();
			SpatialMappingPlugin.Register();
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
			AzureObjectAnchorsPlugin = MakeShared<FAzureObjectAnchorsPlugin>();
			AzureObjectAnchorsPlugin->Register();
			QRTrackingPlugin.Register();
			LocatableCamPlugin.Register();
			SpeechPlugin.Register();
#endif	  // PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#if SUPPORTS_REMOTING
			HolographicRemotingPlugin = MakeShared<FHolographicRemotingPlugin>();
			HolographicRemotingPlugin->Register();
#endif

#if PLATFORM_HOLOLENS
			HolographicWindowAttachmentPlugin.Register();
#endif

			g_MicrosoftOpenXRModule = this;
		}

		void ShutdownModule() override
		{
			g_MicrosoftOpenXRModule = nullptr;

			SpatialAnchorPlugin.Unregister();
			HandMeshPlugin.Unregister();
			SecondaryViewConfigurationPlugin.Unregister();
			SceneUnderstandingPlugin.Unregister();
			SpatialMappingPlugin.Unregister();
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
			AzureObjectAnchorsPlugin->Unregister();
			QRTrackingPlugin.Unregister();
			LocatableCamPlugin.Unregister();
			SpeechPlugin.Unregister();
#endif	  // PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#if SUPPORTS_REMOTING
			HolographicRemotingPlugin->Unregister();
#endif

#if PLATFORM_HOLOLENS
			HolographicWindowAttachmentPlugin.Unregister();
#endif
		}

		FSecondaryViewConfigurationPlugin SecondaryViewConfigurationPlugin;
		FHandMeshPlugin HandMeshPlugin;
		FSpatialAnchorPlugin SpatialAnchorPlugin;
		FSceneUnderstandingPlugin SceneUnderstandingPlugin;
		FSpatialMappingPlugin SpatialMappingPlugin;
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
		TSharedPtr<FAzureObjectAnchorsPlugin> AzureObjectAnchorsPlugin;
		FQRTrackingPlugin QRTrackingPlugin;
		FLocatableCamPlugin LocatableCamPlugin;
		FSpeechPlugin SpeechPlugin;
#endif	  // PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#if SUPPORTS_REMOTING
		TSharedPtr<FHolographicRemotingPlugin> HolographicRemotingPlugin;
#endif

#if PLATFORM_HOLOLENS
		FHolographicWindowAttachmentPlugin HolographicWindowAttachmentPlugin;
#endif
	};
}	 // namespace MicrosoftOpenXR

bool UMicrosoftOpenXRFunctionLibrary::SetUseHandMesh(EHandMeshStatus Mode)
{
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->HandMeshPlugin.Turn(Mode);
}

bool UMicrosoftOpenXRFunctionLibrary::IsQREnabled()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->QRTrackingPlugin.IsEnabled();
#else
	return false;
#endif
}

FTransform UMicrosoftOpenXRFunctionLibrary::GetPVCameraToWorldTransform()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->LocatableCamPlugin.GetCameraTransform();
#else
	return FTransform::Identity;
#endif
}

bool UMicrosoftOpenXRFunctionLibrary::GetPVCameraIntrinsics(FVector2D& focalLength, int& width, int& height,
	FVector2D& principalPoint, FVector& radialDistortion, FVector2D& tangentialDistortion)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->LocatableCamPlugin.GetPVCameraIntrinsics(
		focalLength, width, height, principalPoint, radialDistortion, tangentialDistortion);
#else
	return false;
#endif
}

FVector UMicrosoftOpenXRFunctionLibrary::GetWorldSpaceRayFromCameraPoint(FVector2D pixelCoordinate)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->LocatableCamPlugin.GetWorldSpaceRayFromCameraPoint(pixelCoordinate);
#else
	return FVector::ZeroVector;
#endif
}

bool UMicrosoftOpenXRFunctionLibrary::IsSpeechRecognitionAvailable()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return true;
#endif

	return false;
}

void UMicrosoftOpenXRFunctionLibrary::AddKeywords(TArray<FKeywordInput> Keywords)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MicrosoftOpenXR::g_MicrosoftOpenXRModule->SpeechPlugin.AddKeywords(Keywords);
#endif
}

void UMicrosoftOpenXRFunctionLibrary::RemoveKeywords(TArray<FString> Keywords)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MicrosoftOpenXR::g_MicrosoftOpenXRModule->SpeechPlugin.RemoveKeywords(Keywords);
#endif
}

bool UMicrosoftOpenXRFunctionLibrary::GetPerceptionAnchorFromOpenXRAnchor(void* AnchorID, void** OutPerceptionAnchor)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	if (MicrosoftOpenXR::g_MicrosoftOpenXRModule == nullptr)
	{
		return false;
	}

	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->SpatialAnchorPlugin.GetPerceptionAnchorFromOpenXRAnchor(
		(XrSpatialAnchorMSFT) AnchorID, (::IUnknown**)OutPerceptionAnchor);
#else
	return false;
#endif
}

bool UMicrosoftOpenXRFunctionLibrary::StorePerceptionAnchor(const FString& InPinId, void* InPerceptionAnchor)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	if (MicrosoftOpenXR::g_MicrosoftOpenXRModule == nullptr)
	{
		return false;
	}

	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->SpatialAnchorPlugin.StorePerceptionAnchor(InPinId, (::IUnknown*)InPerceptionAnchor);
#else
	return false;
#endif
}

bool UMicrosoftOpenXRFunctionLibrary::IsRemoting()
{
#if SUPPORTS_REMOTING
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->HolographicRemotingPlugin->IsRemoting();
#endif

	return false;
}

bool UMicrosoftOpenXRFunctionLibrary::CanDetectPlanes()
{
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->SceneUnderstandingPlugin.CanDetectPlanes();
}

bool UMicrosoftOpenXRFunctionLibrary::ToggleAzureObjectAnchors(const bool bOnOff)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	return MicrosoftOpenXR::g_MicrosoftOpenXRModule->AzureObjectAnchorsPlugin->OnToggleARCapture(bOnOff);
#endif

	return true;
}

void UMicrosoftOpenXRFunctionLibrary::InitAzureObjectAnchors(FAzureObjectAnchorSessionConfiguration AOAConfiguration)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MicrosoftOpenXR::g_MicrosoftOpenXRModule->AzureObjectAnchorsPlugin->InitAzureObjectAnchors(AOAConfiguration);
#endif
}

void UMicrosoftOpenXRFunctionLibrary::ResetObjectSearchAreaAroundHead()
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MicrosoftOpenXR::g_MicrosoftOpenXRModule->AzureObjectAnchorsPlugin->ResetObjectSearchAreaAroundHead();
#endif
}

void UMicrosoftOpenXRFunctionLibrary::ResetObjectSearchAreaAroundPoint(FVector Point, float Radius, bool ClearExistingSearchAreas)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	MicrosoftOpenXR::g_MicrosoftOpenXRModule->AzureObjectAnchorsPlugin->ResetObjectSearchAreaAroundPoint(Point, Radius, ClearExistingSearchAreas);
#endif
}

TArray<FARTraceResult> UMicrosoftOpenXRFunctionLibrary::LineTraceTrackedAzureObjectAnchors(const FVector Start, const FVector End)
{
#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
	TArray<FARTraceResult> Results = MicrosoftOpenXR::g_MicrosoftOpenXRModule->AzureObjectAnchorsPlugin->OnLineTraceTrackedObjects(nullptr, Start, End, EARLineTraceChannels::None);
	
	Results.Sort([](const FARTraceResult& A, const FARTraceResult& B)
	{
		return A.GetDistanceFromCamera() < B.GetDistanceFromCamera();
	});

	return Results;
#endif

	return {};
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(MicrosoftOpenXR::FMicrosoftOpenXRModule, MicrosoftOpenXR)
