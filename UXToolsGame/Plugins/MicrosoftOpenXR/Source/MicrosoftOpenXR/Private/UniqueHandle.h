// Copyright (c) 2021 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "OpenXRCommon.h"
#include <assert.h>

namespace MicrosoftOpenXR
{
	template <typename HandleType>
	class TUniqueExtHandle
	{
		using PFN_DestroyFunction = XrResult(XRAPI_PTR*)(HandleType);

	public:
		TUniqueExtHandle() = default;
		TUniqueExtHandle(const TUniqueExtHandle&) = delete;
		TUniqueExtHandle(TUniqueExtHandle&& Other) noexcept
		{
			*this = std::move(Other);
		}

		~TUniqueExtHandle() noexcept
		{
			Reset();
		}

		TUniqueExtHandle& operator=(const TUniqueExtHandle&) = delete;
		TUniqueExtHandle& operator=(TUniqueExtHandle&& Other) noexcept
		{
			if (HandleValue != Other.HandleValue || Destroyer != Other.Destroyer)
			{
				Reset();

				HandleValue = Other.HandleValue;
				Destroyer = Other.Destroyer;

				Other.HandleValue = XR_NULL_HANDLE;
				Other.Destroyer = nullptr;
			}
			return *this;
		}

		operator bool() const noexcept
		{
			return HandleValue != XR_NULL_HANDLE;
		}

		HandleType Handle() const noexcept
		{
			return HandleValue;
		}

		// Extension functions cannot be statically linked, so the creator must pass in the destroy function.
		HandleType* Put(PFN_DestroyFunction DestroyFunction) noexcept
		{
			assert(DestroyFunction != nullptr);
			Reset();
			Destroyer = DestroyFunction;
			return &HandleValue;
		}

		void Reset() noexcept
		{
			if (HandleValue != XR_NULL_HANDLE)
			{
				Destroyer(HandleValue);
				HandleValue = XR_NULL_HANDLE;
			}
			Destroyer = nullptr;
		}

	private:
		HandleType HandleValue{XR_NULL_HANDLE};
		PFN_DestroyFunction Destroyer{nullptr};
	};

	// Defines a move-only smart handle for XrSpace.
	class FSpaceHandle : public MicrosoftOpenXR::TUniqueExtHandle<XrSpace>
	{
	};

	inline FSpaceHandle CreateViewSpace(XrSession Session)
	{
		FSpaceHandle ViewSpace;
		XrReferenceSpaceCreateInfo SpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
		SpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		SpaceCreateInfo.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};
		XR_ENSURE(xrCreateReferenceSpace(Session, &SpaceCreateInfo, ViewSpace.Put(xrDestroySpace)));
		return ViewSpace;
	}

}	 // namespace MicrosoftOpenXR
