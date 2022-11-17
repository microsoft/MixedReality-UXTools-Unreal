// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS

#include "LocatableCamPlugin.h"
#include "InputCoreTypes.h"
#include "OpenXRCore.h"
#include "IOpenXRARModule.h"
#include "IOpenXRARTrackedGeometryHolder.h"
#include "IXRTrackingSystem.h"
#include "OpenXRCameraImageTexture.h"
#include "ARSessionConfig.h"
#include "Misc/CoreDelegates.h"

#include "WindowsMixedRealityInteropUtility.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#include "Windows/PreWindowsApi.h"

#include <DXGI1_4.h>
#include <mfapi.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/windows.Perception.Spatial.Preview.h>
#include <winrt/Windows.Media.Devices.h>
#include <winrt/Windows.Media.Devices.Core.h>

#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"


using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::Perception::Spatial;

using namespace winrt::Windows::Media::Capture;
using namespace winrt::Windows::Media::Capture::Frames;


namespace MicrosoftOpenXR
{

	FLocatableCamPlugin::FLocatableCamPlugin()
	{
	}

	FLocatableCamPlugin::~FLocatableCamPlugin()
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		if (AsyncInfo)
		{
			AsyncInfo.Cancel();
		}
	}

	bool FLocatableCamPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
	{
		OutExtensions.Add(XR_MSFT_SPATIAL_GRAPH_BRIDGE_EXTENSION_NAME);
		return true;
	}

	const void* FLocatableCamPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext)
	{
		XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCreateSpatialGraphNodeSpaceMSFT", (PFN_xrVoidFunction*)&xrCreateSpatialGraphNodeSpaceMSFT));

		static FName SystemName(TEXT("OpenXR"));
		if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
		{
			XRTrackingSystem = GEngine->XRSystem.Get();
		}

		ensure(XRTrackingSystem != nullptr);

		return InNext;
	}

	void FLocatableCamPlugin::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		check(IsInGameThread());

		if (!SharedDXTexture)
		{
			return;
		}

		if (!SharedTextureHolder)
		{
			UE_LOG(LogHMD, Log, TEXT("ARSession isn't started, can't capture frames"));
			return;
		}

		if (Space == XR_NULL_HANDLE && DynamicNode.IsSet())
		{
			XrSpatialGraphNodeSpaceCreateInfoMSFT SpatialGraphNodeSpaceCreateInfo{ XR_TYPE_SPATIAL_GRAPH_NODE_SPACE_CREATE_INFO_MSFT };
			SpatialGraphNodeSpaceCreateInfo.nodeType = XR_SPATIAL_GRAPH_NODE_TYPE_DYNAMIC_MSFT;
			SpatialGraphNodeSpaceCreateInfo.pose = ToXrPose(DynamicNode.GetValue().Value, XRTrackingSystem->GetWorldToMetersScale());

			check(sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId) == sizeof(FGuid));
			winrt::guid SourceGuid = WMRUtility::FGUIDToGuid(DynamicNode.GetValue().Key);
			FMemory::Memcpy(&SpatialGraphNodeSpaceCreateInfo.nodeId, &SourceGuid, sizeof(SpatialGraphNodeSpaceCreateInfo.nodeId));

			XR_ENSURE_MSFT(xrCreateSpatialGraphNodeSpaceMSFT(InSession, &SpatialGraphNodeSpaceCreateInfo, &Space));
		}

		// We leave our pointer null until there's an image to wrap around, so create on demand
		if (SharedTextureHolder->CameraImage == nullptr)
		{
			SharedTextureHolder->CameraImage = NewObject<UOpenXRCameraImageTexture>();
		}
		// This will start the async update process
		SharedTextureHolder->CameraImage->Init(SharedDXTexture);
		SharedDXTexture = nullptr;

		PVCameraToWorldMatrix = FTransform::Identity;

		if (Space != XR_NULL_HANDLE)
		{
			XrSpaceLocation SpaceLocation{ XR_TYPE_SPACE_LOCATION };
			XR_ENSURE_MSFT(xrLocateSpace(Space, TrackingSpace, DisplayTime, &SpaceLocation));
			const XrSpaceLocationFlags ValidFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT;
			if ((SpaceLocation.locationFlags & ValidFlags) == ValidFlags)
			{
				PVCameraToWorldMatrix = ToFTransform(SpaceLocation.pose, XRTrackingSystem->GetWorldToMetersScale()) * XRTrackingSystem->GetTrackingToWorldTransform();
			}
		}
	}

	void FLocatableCamPlugin::Register()
	{
		IModularFeatures::Get().RegisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));

		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddLambda([this]()
		{
			if (IsCameraCaptureDesired)
			{
				OnToggleARCapture(true);
			}
		});

		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddLambda([this]()
		{
			if (IsCameraCaptureDesired)
			{
				StopCameraCapture();
			}
		});
	}

	void FLocatableCamPlugin::Unregister()
	{
		StopCameraCapture();
		IModularFeatures::Get().UnregisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));

		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
	}

	void FLocatableCamPlugin::StartCameraCapture(int DesiredWidth, int DesiredHeight, int DesiredFPS)
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		check(IsInGameThread());
		if (CameraFrameReader)
		{
			UE_LOG(LogHMD, Log, TEXT("Camera is already capturing frames. Aborting."));
			return;
		}

		auto FindAllAsyncOp = MediaFrameSourceGroup::FindAllAsync();
		AsyncInfo = FindAllAsyncOp;
		FindAllAsyncOp.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
		{
			std::lock_guard<std::recursive_mutex> lock(RefsLock);
			if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Canceled)
			{
				// Do not reset AsyncInfo reference here since the function that cancelled this already reassigned it.
				return;
			}
			else if (asyncStatus != winrt::Windows::Foundation::AsyncStatus::Completed)
			{
				AsyncInfo = nullptr;
				return;
			}
			AsyncInfo = nullptr;

			auto DiscoveredGroups = asyncInfo.GetResults();
			MediaFrameSourceGroup ChosenSourceGroup = nullptr;
			MediaFrameSourceInfo ChosenSourceInfo = nullptr;

			MediaCaptureInitializationSettings CaptureSettings = MediaCaptureInitializationSettings();
			CaptureSettings.StreamingCaptureMode(StreamingCaptureMode::Video);
			CaptureSettings.MemoryPreference(MediaCaptureMemoryPreference::Auto); // For GPU
			CaptureSettings.VideoProfile(nullptr);

			for(auto&& Group : DiscoveredGroups)
			{
				// For HoloLens, use the video conferencing video profile - this will give the best power consumption.
				auto profileList = MediaCapture::FindKnownVideoProfiles(Group.Id(), KnownVideoProfile::VideoConferencing);
				if (profileList.Size() == 0)
				{
					// No video conferencing profiles in this group, move to the next one.
					continue;
				}

				// Cache the first valid group and profile in case we do not find a profile that matches the input description.
				if (ChosenSourceGroup == nullptr)
				{
					ChosenSourceGroup = Group;

					CaptureSettings.SourceGroup(ChosenSourceGroup);
					CaptureSettings.VideoProfile(profileList.GetAt(0));
				}

				if (DesiredWidth > 0 && DesiredHeight > 0 && DesiredFPS > 0)
				{
					bool found = false;
					for (auto&& profile : profileList)
					{
						for (auto&& desc : profile.SupportedRecordMediaDescription())
						{
							// Check for a profile that matches our desired dimensions.
							if (desc.Width() == DesiredWidth && desc.Height() == DesiredHeight && desc.FrameRate() == DesiredFPS)
							{
								ChosenSourceGroup = Group;

								CaptureSettings.SourceGroup(Group);
								CaptureSettings.VideoProfile(profile);
								CaptureSettings.RecordMediaDescription(desc);
								found = true;

								break;
							}
						}
						if (found)
						{
							break;
						}
					}
				}
			}

			// If there was no camera available, then log it and bail
			if (ChosenSourceGroup == nullptr)
			{
				UE_LOG(LogHMD, Log, TEXT("No media frame source found, so no camera images will be delivered"));
				return;
			}

			if (DesiredWidth > 0 && DesiredHeight > 0 && DesiredFPS > 0	&& CaptureSettings.RecordMediaDescription() == nullptr)
			{
				UE_LOG(LogHMD, Log, TEXT("No matching video format found, using default profile instead."));
			}

			// Find the color camera source
			// Search through the infos to determine if this is the color camera source
			for (auto&& Info : ChosenSourceGroup.SourceInfos())
			{
				if (Info.SourceKind() == MediaFrameSourceKind::Color)
				{
					ChosenSourceInfo = Info;
					break;
				}
			}

			// If there was no camera available, then log it and bail
			if (ChosenSourceInfo == nullptr)
			{
				UE_LOG(LogHMD, Log, TEXT("No media frame source info found, so no camera images will be delivered"));
				return;
			}

			// Create our capture object with our settings
			winrt::agile_ref < winrt::Windows::Media::Capture::MediaCapture > Capture{ MediaCapture() };
			auto InitializeAsyncOp = Capture.get().InitializeAsync(CaptureSettings);
			AsyncInfo = InitializeAsyncOp;
			InitializeAsyncOp.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
			{
				std::lock_guard<std::recursive_mutex> lock(RefsLock);
				if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Canceled)
				{
					// Do not reset AsyncInfo reference here since the function that cancelled this already reassigned it.
					return;
				}
				else if (asyncStatus != winrt::Windows::Foundation::AsyncStatus::Completed)
				{
					AsyncInfo = nullptr;
					UE_LOG(LogHMD, Log, TEXT("Failed to open camera, please check Webcam capability"));
					return;
				}
				AsyncInfo = nullptr;

				// Get the frame source from the source info we got earlier
				MediaFrameSource FrameSource = Capture.get().FrameSources().Lookup(ChosenSourceInfo.Id());

				// Now create and start the frame reader
				auto CreateFrameReaderAsyncOp = Capture.get().CreateFrameReaderAsync(FrameSource);
				AsyncInfo = CreateFrameReaderAsyncOp;
				CreateFrameReaderAsyncOp.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
				{
					std::lock_guard<std::recursive_mutex> lock(RefsLock);
					if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Canceled)
					{
						// Do not reset AsyncInfo reference here since the function that cancelled this already reassigned it.
						return;
					}
					else if (asyncStatus != winrt::Windows::Foundation::AsyncStatus::Completed)
					{
						AsyncInfo = nullptr;
						return;
					}
					AsyncInfo = nullptr;

					MediaFrameReader FrameReader = asyncInfo.GetResults();
					auto StartAsyncOp = FrameReader.StartAsync();
					AsyncInfo = StartAsyncOp;
					StartAsyncOp.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
					{
						std::lock_guard<std::recursive_mutex> lock(RefsLock);
						// Finally, copy to our object
						if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Canceled)
						{
							// Do not reset AsyncInfo reference here since the function that cancelled this already reassigned it.
							return;
						}
						else if (asyncStatus != winrt::Windows::Foundation::AsyncStatus::Completed)
						{
							AsyncInfo = nullptr;
							return;
						}
						AsyncInfo = nullptr;

						MediaFrameReaderStartStatus StartStatus = asyncInfo.GetResults();
						if (StartStatus == MediaFrameReaderStartStatus::Success)
						{
							std::lock_guard<std::recursive_mutex> lock(RefsLock);
							CameraCapture = std::move(Capture);
							CameraFrameReader = std::move(FrameReader);
							CameraFrameSource = std::move(FrameSource);
							UE_LOG(LogHMD, Log, TEXT("Successfully created the camera reader"));

							// Subscribe the inbound frame event
							OnFrameArrivedEvent = CameraFrameReader.FrameArrived(winrt::auto_revoke, [this](auto&& sender, auto&& args) { OnFrameArrived(sender, args); });
						}
						else
						{
							UE_LOG(LogHMD, Log, TEXT("Failed to start the frame reader with status = %d"), static_cast<int>(StartStatus));
						}
					});
				});
			});
		});
	}

	void FLocatableCamPlugin::StopCameraCapture()
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		check(IsInGameThread());

		OnFrameArrivedEvent.revoke();
		if (AsyncInfo)
		{
			AsyncInfo.Cancel();
		}

		CameraIntrinsics = nullptr;
		DynamicNode.Reset();
		SharedDXTexture = nullptr;
		if (Space != XR_NULL_HANDLE)
		{
			xrDestroySpace(Space);
			Space = XR_NULL_HANDLE;
		}

		if (CameraFrameReader)
		{
			auto StopAsyncOp = CameraFrameReader.StopAsync();
			AsyncInfo = StopAsyncOp;
			StopAsyncOp.Completed([=](auto&& asyncInfo, auto&& asyncStatus)
			{
				if (asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed)
				{
					std::lock_guard<std::recursive_mutex> lock(RefsLock);
					AsyncInfo = nullptr;
					CameraCapture = nullptr;
					CameraFrameReader = nullptr;
					CameraFrameSource = nullptr;
				}
			});
		}
	}


	void FLocatableCamPlugin::OnStartARSession(class UARSessionConfig* SessionConfig)
	{
		Format = SessionConfig->GetDesiredVideoFormat();
		SharedTextureHolder = MakeShared<FSharedTextureHolder>();
	}

	void FLocatableCamPlugin::OnStopARSession()
	{
		SharedTextureHolder = nullptr;
	}


	TOptional<TPair<FGuid, FTransform>> FindDynamicNode(const MediaFrameReference& frame, float WorldToMetersScale) 
	{
		// Copied MFStreamExtension_CameraExtrinsics's value from mfapi.h so that we don't have to link against Mfplat.lib.
		// mfapi.h is still needed for the MFCameraExtrinsics struct.
		constexpr winrt::guid CameraExtrinsicsGuid(0x6b761658, 0xb7ec, 0x4c3b, { 0x82, 0x25, 0x86, 0x23, 0xca, 0xbe, 0xc3, 0x1d });
		if (auto inspectable = frame.Properties().TryLookup(CameraExtrinsicsGuid)) 
		{
			auto propertyValue = inspectable.try_as<winrt::Windows::Foundation::IPropertyValue>();
			if (propertyValue && propertyValue.Type() == winrt::Windows::Foundation::PropertyType::UInt8Array) 
			{
				winrt::com_array<uint8_t> bytes;
				propertyValue.GetUInt8Array(bytes);
				if (bytes.size() >= sizeof(MFCameraExtrinsics)) 
				{
					const auto* const extrinsics = reinterpret_cast<const MFCameraExtrinsics*>(bytes.data());
					if (extrinsics->TransformCount > 0) 
					{
						const MFCameraExtrinsic_CalibratedTransform& transform = extrinsics->CalibratedTransforms[0];

						DirectX::XMFLOAT4 rot(transform.Orientation.x, transform.Orientation.y, transform.Orientation.z, transform.Orientation.w);
						DirectX::XMFLOAT3 pos(transform.Position.x, transform.Position.y, transform.Position.z);

						FTransform TrackingSpaceTransform(WMRUtility::FromMixedRealityQuaternion(rot), WMRUtility::FromMixedRealityVector(pos) * WorldToMetersScale);

						return MakeTuple(WMRUtility::GUIDToFGuid(transform.CalibrationId), TrackingSpaceTransform);
					}
				}
			}
		}
		return {};
	}

	void FLocatableCamPlugin::OnFrameArrived(MediaFrameReader SendingFrameReader, MediaFrameArrivedEventArgs FrameArrivedArgs)
	{
		MediaFrameReference CurrentFrame = SendingFrameReader.TryAcquireLatestFrame();
		if (CurrentFrame == nullptr)
		{
			return;
		}

		// Drill down through the objects to get the underlying D3D texture
		VideoMediaFrame VideoFrame = CurrentFrame.VideoMediaFrame();
		auto ManagedSurface = VideoFrame.Direct3DSurface();

		if (ManagedSurface == nullptr)
		{
			UE_LOG(LogHMD, Log, TEXT("OnFrameArrived() : VideoMediaFrame->Direct3DSurface was null, so no image to process"));
			return;
		}

		winrt::com_ptr<IDXGIResource1> srcResource = nullptr;
		winrt::com_ptr<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> DxgiInterfaceAccess =
			ManagedSurface.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();

		if (DxgiInterfaceAccess == nullptr)
		{
			UE_LOG(LogHMD, Log, TEXT("OnFrameArrived() : Failed to get DxgiInterfaceAccess from ManagedSurface.  Cannot process image."));
			return;
		}

		DxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(srcResource.put()));

		if (srcResource == nullptr)
		{
			UE_LOG(LogHMD, Log, TEXT("Unable to get the underlying video texture"));
			return;
		}

		{
			std::lock_guard<std::recursive_mutex> lock(RefsLock);
			if (CameraFrameReader == nullptr)
			{
				return;
			}
			// Get camera intrinsics, since we just have the one camera, cache the intrinsics.
			if (CameraIntrinsics == nullptr)
			{
				CameraIntrinsics = VideoFrame.CameraIntrinsics();
			}

			// Find current frame's tracking information from the frame's coordinate system.
			if (!DynamicNode)
			{
				DynamicNode = FindDynamicNode(CurrentFrame, XRTrackingSystem->GetWorldToMetersScale());
			}
			auto OutSharedDXTexture = std::make_shared<winrt::handle>();
			if (FAILED(srcResource->CreateSharedHandle(NULL, DXGI_SHARED_RESOURCE_READ, NULL, OutSharedDXTexture->put())))
			{
				UE_LOG(LogHMD, Log, TEXT("Unable to create shared handler of the video texture"));
				return;
			}
			SharedDXTexture = OutSharedDXTexture;
		}
	}


	void FLocatableCamPlugin::FSharedTextureHolder::AddReferencedObjects(FReferenceCollector& Collector)
	{
		Collector.AddReferencedObject(CameraImage);
	}


	FTransform FLocatableCamPlugin::GetCameraTransform() const
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		return PVCameraToWorldMatrix;
	}


	bool FLocatableCamPlugin::GetPVCameraIntrinsics(FVector2D& focalLength, int& width, int& height, FVector2D& principalPoint, FVector& radialDistortion, FVector2D& tangentialDistortion) const
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		if (CameraIntrinsics == nullptr)
		{
			return false;
		}

		focalLength = WMRUtility::FromFloat2(CameraIntrinsics.FocalLength());
		width = CameraIntrinsics.ImageWidth();
		height = CameraIntrinsics.ImageHeight();
		principalPoint = WMRUtility::FromFloat2(CameraIntrinsics.PrincipalPoint());
		radialDistortion = WMRUtility::FromFloat3(CameraIntrinsics.RadialDistortion(), XRTrackingSystem->GetWorldToMetersScale());
		tangentialDistortion = WMRUtility::FromFloat2(CameraIntrinsics.TangentialDistortion());

		return true;
	}


	FVector FLocatableCamPlugin::GetWorldSpaceRayFromCameraPoint(FVector2D pixelCoordinate) const
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		if (CameraIntrinsics == nullptr)
		{
			return FVector::ZeroVector;
		}

		auto unprojectedPointAtUnitDepth = CameraIntrinsics.UnprojectAtUnitDepth(winrt::Windows::Foundation::Point(pixelCoordinate.X, pixelCoordinate.Y));

		FVector ray = WMRUtility::FromFloat3(
			winrt::Windows::Foundation::Numerics::float3(
				unprojectedPointAtUnitDepth,
				-1.0f // Unprojection happened at 1 meter
			)
			, XRTrackingSystem->GetWorldToMetersScale());

		ray.Normalize();

		return PVCameraToWorldMatrix.TransformVector(ray);
	}

	bool FLocatableCamPlugin::OnGetCameraIntrinsics(FARCameraIntrinsics& OutCameraIntrinsics) const
	{ 
		FVector radialDistortion;
		FVector2D tangentialDistortion;
		return GetPVCameraIntrinsics(OutCameraIntrinsics.FocalLength, OutCameraIntrinsics.ImageResolution.X, OutCameraIntrinsics.ImageResolution.Y, OutCameraIntrinsics.PrincipalPoint, radialDistortion, tangentialDistortion);
	}

	UARTexture* FLocatableCamPlugin::OnGetARTexture(EARTextureType TextureType) const
	{ 
		if (SharedTextureHolder && TextureType == EARTextureType::CameraImage)
		{
			return SharedTextureHolder->CameraImage;
		}
		return nullptr;
	}

	bool FLocatableCamPlugin::OnToggleARCapture(const bool bOnOff) 
	{
		if (UMicrosoftOpenXRFunctionLibrary::IsRemoting())
		{
			UE_LOG(LogHMD, Warning, TEXT("Camera ARCapture is not supported over remoting."));
			return false;
		}

		IsCameraCaptureDesired = bOnOff;

		if (bOnOff)
		{
			StartCameraCapture(Format.Width, Format.Height, Format.FPS);
		}
		else
		{
			StopCameraCapture();
		}
		return true;
	}

	bool FLocatableCamPlugin::IsEnabled() const
	{
		std::lock_guard<std::recursive_mutex> lock(RefsLock);
		return CameraFrameReader != nullptr;
	}


	IOpenXRCustomCaptureSupport* FLocatableCamPlugin::GetCustomCaptureSupport(const EARCaptureType CaptureType)
	{
		if (CaptureType == EARCaptureType::Camera)
		{
			return this;
		}
		return nullptr;
	}


}	 // namespace MicrosoftOpenXR

#endif //PLATFORM_WINDOWS || PLATFORM_HOLOLENS
