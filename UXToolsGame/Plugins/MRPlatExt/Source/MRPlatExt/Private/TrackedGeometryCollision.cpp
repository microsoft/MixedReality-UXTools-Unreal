#include "TrackedGeometryCollision.h"

namespace MRPlatExt
{
	TrackedGeometryCollision::TrackedGeometryCollision(const TArray<FVector> InVertices, const TArray<MRMESH_INDEX_TYPE> InIndices)
	{
		if (InVertices.Num() == 0)
		{
			return;
		}

		Vertices = std::move(InVertices);
		Indices = std::move(InIndices);

		// Create a bounding box from the input vertices to reduce the number of full meshes that need to be hit-tested.
		BoundingBox = FBox(&Vertices[0], Vertices.Num());
	}

	void TrackedGeometryCollision::UpdateVertices(const TArray<FVector> InVertices, const TArray<MRMESH_INDEX_TYPE> InIndices)
	{
		Vertices = InVertices;
		Indices = InIndices;

		// Create a bounding box from the input vertices to reduce the number of full meshes that need to be hit-tested.
		BoundingBox = FBox(&Vertices[0], Vertices.Num());
	}

	bool TrackedGeometryCollision::Collides(const FVector Start, const FVector End, const FTransform MeshToWorld, FVector& OutHitPoint, FVector& OutHitNormal, float& OutHitDistance)
	{
		if (MeshToWorld.GetScale3D().IsNearlyZero())
		{
			return false;
		}

		// Check bounding box collision first so we don't check triangles for meshes we definitely won't collide with.
		if (!FMath::LineBoxIntersection(BoundingBox.TransformBy(MeshToWorld), Start, End, End - Start))
		{
			return false;
		}

		// Check for triangle collision and set the output hit position, normal, and distance.
		for (int i = 0; i < Indices.Num(); i += 3)
		{
			// Ignore this triangle if it has indices out of range.
			if ((unsigned int)Indices[i] > (unsigned int)Vertices.Num()
				|| (unsigned int)Indices[i + 1] > (unsigned int)Vertices.Num()
				|| (unsigned int)Indices[i + 2] > (unsigned int)Vertices.Num())
			{
				continue;
			}

			if (FMath::SegmentTriangleIntersection(Start, End,
				MeshToWorld.TransformPosition(Vertices[Indices[i]]),
				MeshToWorld.TransformPosition(Vertices[Indices[i + 1]]),
				MeshToWorld.TransformPosition(Vertices[Indices[i + 2]]),
				OutHitPoint, OutHitNormal))
			{
				OutHitDistance = (OutHitPoint - Start).Size();
				return true;
			}
		}

		return false;
	}
}  // namespace MRPlatExt