// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

/** Utility class for constructing a cone-shaped mesh for proximity detection.
 *
 *  The volume is constructed radially symmetric around the forward axis.
 *  The near face is a simple disc, while the front is a spherical section.
 *  The tip of the cylinder is the origin of the mesh.
 *
 *
 *               *------
 *              *       ------
 *             *              ------
 *             *                    ------
 *            *                           |
 *            * Far                       |  Near         Tip
 * Axis <-----*---------------------------|----------------o
 *
 */
class FUxtHandProximityMeshData
{
public:
	// Populate buffers with mesh data based on cone parameters.
	void Build(float ConeAngle, float ConeOffset, float ConeSideLength, int32 NewNumSegments);

	// Update a mesh section of the procedural mesh
	void UpdateMesh(UProceduralMeshComponent* Mesh, int32 Section) const;

	// Set to true if normals and UVs are needed.
	bool bEnableLighting = false;

private:
	// Add a single vertex on the forward axis.
	template <typename UVMapFunc>
	int32 AddVertex(float Offset, bool FaceForward, UVMapFunc UVMap, bool bAddToConvexHull);

	// Create a ring of vertices around the forward axis.
	template <typename UVMapFunc>
	int32 AddVertexRing(float Radius, float Offset, float AngleFromForward, UVMapFunc UVMap, bool bAddToConvexHull);

	// Connect a center vertex and a vertex ring with a triangle fan.
	void AddTriangleRing(int32 RingStartA, int32 RingStartB);

	// Connect two vertex rings with a closed triangle strip.
	// FaceForward determines the winding and thus normal orientation.
	void AddTriangleFan(int32 Center, int32 RingStart, bool FaceForward);

	// Buffers
	TArray<FVector> Vertices;
	TArray<FVector> ConvexHull;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	// Number of cone segments around the forward axis.
	int32 NumSegments = 0;
	// Number of rings of the spherical far end surface.
	int32 NumRings = 0;
	// Angle between cone segments.
	float SegmentAngle = 0.0f;
	// Angle between rings.
	float RingAngle = 0.0f;
};
