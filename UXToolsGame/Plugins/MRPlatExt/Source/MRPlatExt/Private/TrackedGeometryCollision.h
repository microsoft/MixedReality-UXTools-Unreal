#pragma once
#include "OpenXRCommon.h"
#include "OpenXRCore.h"
#include "IOpenXRARModule.h"
#include "IOpenXRARTrackedGeometryHolder.h"

#include "HeadMountedDisplayTypes.h"
#include "ARTypes.h"

namespace MRPlatExt
{
	class TrackedGeometryCollision
	{
	public:
		TrackedGeometryCollision(const TArray<FVector> InVertices, const TArray<MRMESH_INDEX_TYPE> InIndices);
		
		void UpdateVertices(const TArray<FVector> InVertices, const TArray<MRMESH_INDEX_TYPE> InIndices);

		/// <summary>
		/// Hit test a ray against tracked mesh data.
		/// </summary>
		/// <param name="Start">Start of collision ray in world space</param>
		/// <param name="End">End of collision ray in world space</param>
		/// <param name="TrackingToWorld">Transform from mesh local space to world space.  The mesh may not be in tracking space.</param>
		/// <param name="OutHitPoint">Position of hit in world space</param>
		/// <param name="OutHitNormal">Normal of hit in world space</param>
		/// <param name="OutHitDistance">Distance from ray start</param>
		/// <returns>True if the input ray collides with this mesh.</returns>
		bool Collides(const FVector Start, const FVector End, const FTransform MeshToWorld, FVector& OutHitPoint, FVector& OutHitNormal, float& OutHitDistance);

	private:
		TArray<FVector> Vertices;
		TArray<MRMESH_INDEX_TYPE> Indices;

		FBox BoundingBox;
	};
}  // namespace MRPlatExt