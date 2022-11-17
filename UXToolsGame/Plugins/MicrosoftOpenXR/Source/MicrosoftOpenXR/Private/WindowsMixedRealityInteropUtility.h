// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#if PLATFORM_WINDOWS || PLATFORM_HOLOLENS
#include "CoreMinimal.h"
#include "HAL\UnrealMemory.h"

#include "Windows/WindowsHWrapper.h"
#include "HeadMountedDisplayTypes.h"

#include <DirectXMath.h>
#include <unknwn.h>
#include <winrt/base.h>


namespace MicrosoftOpenXR
{
	class WMRUtility
	{
	public:
		// Convert between DirectX XMMatrix to Unreal FMatrix.
		static FORCEINLINE FMatrix ToFMatrix(DirectX::XMMATRIX& M)
		{
			DirectX::XMFLOAT4X4 dst;
			DirectX::XMStoreFloat4x4(&dst, M);

			return FMatrix(
				FPlane(dst._11, dst._21, dst._31, dst._41),
				FPlane(dst._12, dst._22, dst._32, dst._42),
				FPlane(dst._13, dst._23, dst._33, dst._43),
				FPlane(dst._14, dst._24, dst._34, dst._44));
		}

		static FORCEINLINE FMatrix ToFMatrix(DirectX::XMFLOAT4X4& M)
		{
			return FMatrix(
				FPlane(M._11, M._21, M._31, M._41),
				FPlane(M._12, M._22, M._32, M._42),
				FPlane(M._13, M._23, M._33, M._43),
				FPlane(M._14, M._24, M._34, M._44));
		}

		static FORCEINLINE FTransform FromMixedRealityTransform(const DirectX::XMMATRIX& M, float InScale = 1.0f)
		{
			DirectX::XMVECTOR Scale;
			DirectX::XMVECTOR Rotation;
			DirectX::XMVECTOR Translation;
			DirectX::XMMatrixDecompose(&Scale, &Rotation, &Translation, M);

			return FTransform(FromXMVectorRotation(Rotation), FromXMVectorTranslation(Translation, InScale), FromXMVectorScale(Scale));
		}

		static FORCEINLINE FVector FromMixedRealityVector(DirectX::XMFLOAT3 pos)
		{
			return FVector(
				-1.0f * pos.z,
				pos.x,
				pos.y);
		}
		
		static FORCEINLINE DirectX::XMFLOAT3 ToMixedRealityVector(FVector pos)
		{
			return DirectX::XMFLOAT3(
				pos.Y,
				pos.Z,
				-1.0f * pos.X);
		}

		static FORCEINLINE FVector FromMixedRealityScale(DirectX::XMFLOAT3 pos)
		{
			return FVector(
				pos.z,
				pos.x,
				pos.y);
		}

		static FORCEINLINE FVector FromMixedRealityScale(winrt::Windows::Foundation::Numerics::float3 pos)
		{
			return FVector(
				pos.z,
				pos.x,
				pos.y);
		}

		static FORCEINLINE DirectX::XMFLOAT3 ToMixedRealityScale(FVector pos)
		{
			return DirectX::XMFLOAT3(
				pos.Y,
				pos.Z,
				pos.X);
		}


		static FORCEINLINE FQuat FromMixedRealityQuaternion(DirectX::XMFLOAT4 rot)
		{
			FQuat quaternion(
				-1.0f * rot.z,
				rot.x,
				rot.y,
				-1.0f * rot.w);
			quaternion.Normalize();

			return quaternion;
		}

		static FORCEINLINE DirectX::XMFLOAT4 ToMixedRealityQuaternion(FQuat rot)
		{
			// Windows api IsNormalized checks fail on a negative identity quaternion.
			if (rot == FQuat::Identity)
			{
				return DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
			}

			DirectX::XMVECTOR v = DirectX::XMVectorSet(
				rot.Y,
				rot.Z,
				-1.0f * rot.X,
				-1.0f * rot.W);
			DirectX::XMQuaternionNormalize(v);

			DirectX::XMFLOAT4 quatf4out;
			DirectX::XMStoreFloat4(&quatf4out, v);
			return quatf4out;
		}

		static FORCEINLINE FVector FromFloat3(winrt::Windows::Foundation::Numerics::float3 pos, float scale = 1.0f)
		{
			return FVector(
				-1.0f * pos.z,
				pos.x,
				pos.y) * scale;
		}

		static FORCEINLINE FVector3f FromFloat3ToFVector3f(winrt::Windows::Foundation::Numerics::float3 pos, float scale = 1.0f)
		{
			return FVector3f(
				-1.0f * pos.z,
				pos.x,
				pos.y) * scale;
		}

		static FORCEINLINE FVector FromXMVectorTranslation(DirectX::XMVECTOR InValue, float scale = 1.0f)
		{
			InValue = DirectX::XMVectorMultiply(InValue, DirectX::XMVectorSet(scale, scale, -1 * scale, scale));
			InValue = DirectX::XMVectorSwizzle(InValue, 2, 0, 1, 3);

			DirectX::XMFLOAT3 Dest;
			DirectX::XMStoreFloat3(&Dest, InValue);
			return FVector(Dest.x, Dest.y, Dest.z);
		}

		static FORCEINLINE FQuat FromXMVectorRotation(DirectX::XMVECTOR InValue)
		{
			DirectX::XMFLOAT4 Dest;
			DirectX::XMStoreFloat4(&Dest, InValue);

			return FromMixedRealityQuaternion(Dest);
		}

		static FORCEINLINE FVector FromXMVectorScale(DirectX::XMVECTOR InValue)
		{
			InValue = DirectX::XMVectorSwizzle(InValue, 2, 0, 1, 3);

			DirectX::XMFLOAT3 Dest;
			DirectX::XMStoreFloat3(&Dest, InValue);
			return FVector(Dest.x, Dest.y, Dest.z);
		}

		static FORCEINLINE FVector2D FromFloat2(winrt::Windows::Foundation::Numerics::float2 pos)
		{
			return FVector2D(pos.x, pos.y);
		}



		static FORCEINLINE FGuid GUIDToFGuid(const winrt::guid& InGuid)
		{
			// Pack the FGuid correctly from the input winrt guid data, so the value is not lost in conversion.
			// FGuids use 4 uint32's
			// winrt guids use uint32, uint16, uint16, uint8[8]
			// Do not simply memcpy guid to FGuid, or B, C, and D will be mismatched.

			uint32 A = InGuid.Data1;
			uint32 B = (uint32)InGuid.Data2 << 16 | InGuid.Data3;
			uint32 C = (uint32)InGuid.Data4[0] << 24 |
				(uint32)InGuid.Data4[1] << 16 |
				(uint16)InGuid.Data4[2] << 8 |
				InGuid.Data4[3];
			uint32 D = (uint32)InGuid.Data4[4] << 24 |
				(uint32)InGuid.Data4[5] << 16 |
				(uint16)InGuid.Data4[6] << 8 |
				InGuid.Data4[7];

			return FGuid(A, B, C, D);
		}

		static FORCEINLINE winrt::guid FGUIDToGuid(const FGuid& InGuid)
		{
			// Pack the winrt guid correctly from the input FGuid data, so the value is not lost in conversion.
			// FGuids use 4 uint32's
			// winrt guids use uint32, uint16, uint16, uint8[8]
			// Do not simply memcpy FGuid to guid, or Data2, Data3, and Data4 will be mismatched.

			uint32_t Data1 = InGuid.A;
			uint16_t Data2 = (uint16_t)((InGuid.B & 0xFFFF0000) >> 16);
			uint16_t Data3 = (uint16_t)(InGuid.B & 0x0000FFFF);

			std::array<uint8_t, 8Ui64> Data4;
			Data4[0] = (uint8_t)((InGuid.C & 0xFF000000) >> 24);
			Data4[1] = (uint8_t)((InGuid.C & 0x00FF0000) >> 16);
			Data4[2] = (uint8_t)((InGuid.C & 0x0000FF00) >> 8);
			Data4[3] = (uint8_t)(InGuid.C & 0x000000FF);

			Data4[4] = (uint8_t)((InGuid.D & 0xFF000000) >> 24);
			Data4[5] = (uint8_t)((InGuid.D & 0x00FF0000) >> 16);
			Data4[6] = (uint8_t)((InGuid.D & 0x0000FF00) >> 8);
			Data4[7] = (uint8_t)(InGuid.D & 0x000000FF);

			return winrt::guid(Data1, Data2, Data3, Data4);
		}
	};
}


#endif

