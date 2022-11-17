// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "TrackedGeometryCollision.h"

namespace MicrosoftOpenXR
{
	TrackedGeometryCollision::TrackedGeometryCollision(TArray<FVector3f> InVertices, TArray<MRMESH_INDEX_TYPE> InIndices)
		: Indices(std::move(InIndices))
	{
		{
			const int Num = InVertices.Num();
			Vertices.Reserve(Num);
			for (int i = 0; i < Num; ++i)
			{
				Vertices.Add(FVector(InVertices[i]));
			}
		}
		// Create a bounding box from the input vertices to reduce the number of full meshes that need to be hit-tested.
		if (Vertices.Num() > 0)
		{
			BoundingBox = FBox(&Vertices[0], Vertices.Num());
		}
	}

	bool TrackedGeometryCollision::Collides(const FVector Start, const FVector End, const FTransform MeshToWorld,
		FVector& OutHitPoint, FVector& OutHitNormal, float& OutHitDistance)
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
			if ((unsigned int) Indices[i] > (unsigned int) Vertices.Num() ||
				(unsigned int) Indices[i + 1] > (unsigned int) Vertices.Num() ||
				(unsigned int) Indices[i + 2] > (unsigned int) Vertices.Num())
			{
				continue;
			}

			if (FMath::SegmentTriangleIntersection(Start, End, MeshToWorld.TransformPosition(Vertices[Indices[i]]),
					MeshToWorld.TransformPosition(Vertices[Indices[i + 1]]),
					MeshToWorld.TransformPosition(Vertices[Indices[i + 2]]), OutHitPoint, OutHitNormal))
			{
				OutHitDistance = (OutHitPoint - Start).Size();
				return true;
			}
		}

		return false;
	}

	void TrackedGeometryCollision::CreateMeshDataForBoundingBox(FVector3f Center, FVector3f HalfExtents, TArray<FVector3f>& OutVertices, TArray<MRMESH_INDEX_TYPE>& OutIndices)
	{
		// Ensure output arrays are empty.  
		OutVertices.Empty();
		OutIndices.Empty();

		// Top Vertices (+Z)
		OutVertices.Add(Center + HalfExtents);
		OutVertices.Add(Center + FVector3f(-HalfExtents.X, HalfExtents.Y, HalfExtents.Z));
		OutVertices.Add(Center + FVector3f(HalfExtents.X, -HalfExtents.Y, HalfExtents.Z));
		OutVertices.Add(Center + FVector3f(-HalfExtents.X, -HalfExtents.Y, HalfExtents.Z));

		// Bottom Vertices (-Z)
		OutVertices.Add(Center + FVector3f(HalfExtents.X, HalfExtents.Y, -HalfExtents.Z));
		OutVertices.Add(Center + FVector3f(-HalfExtents.X, HalfExtents.Y, -HalfExtents.Z));
		OutVertices.Add(Center + FVector3f(HalfExtents.X, -HalfExtents.Y, -HalfExtents.Z));
		OutVertices.Add(Center - HalfExtents);

		// Clockwise winding
		//Top
		OutIndices.Add(0);
		OutIndices.Add(1);
		OutIndices.Add(2);

		OutIndices.Add(2);
		OutIndices.Add(1);
		OutIndices.Add(3);

		// Front
		OutIndices.Add(1);
		OutIndices.Add(5);
		OutIndices.Add(3);

		OutIndices.Add(3);
		OutIndices.Add(5);
		OutIndices.Add(7);

		// Bottom
		OutIndices.Add(6);
		OutIndices.Add(5);
		OutIndices.Add(4);

		OutIndices.Add(5);
		OutIndices.Add(6);
		OutIndices.Add(7);

		// Back
		OutIndices.Add(0);
		OutIndices.Add(2);
		OutIndices.Add(4);

		OutIndices.Add(2);
		OutIndices.Add(6);
		OutIndices.Add(4);

		// Left
		OutIndices.Add(2);
		OutIndices.Add(3);
		OutIndices.Add(6);

		OutIndices.Add(3);
		OutIndices.Add(7);
		OutIndices.Add(6);

		// Right
		OutIndices.Add(1);
		OutIndices.Add(0);
		OutIndices.Add(4);

		OutIndices.Add(1);
		OutIndices.Add(4);
		OutIndices.Add(5);
	}
}	 // namespace MicrosoftOpenXR
