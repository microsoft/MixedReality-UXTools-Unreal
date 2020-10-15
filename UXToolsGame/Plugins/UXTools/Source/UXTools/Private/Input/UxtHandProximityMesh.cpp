// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#include "Input/UxtHandProximityMesh.h"

#include "ProceduralMeshComponent.h"

#include "Math/UnrealMathUtility.h"

// Populate buffers with mesh data based on cone parameters.
void FUxtHandProximityMeshData::Build(float ConeAngle, float ConeOffset, float ConeSideLength, int32 NewNumSegments)
{
	// Convert parameters to more practical values
	const float ConeAngleRadians = FMath::DegreesToRadians(ConeAngle);
	const float SinConeAngle = FMath::Sin(ConeAngleRadians);
	const float CosConeAngle = FMath::Cos(ConeAngleRadians);
	const float TanConeAngle = FMath::Tan(ConeAngleRadians);

	const float NearPlane = ConeOffset;
	const float FarRadius = ConeSideLength + ConeOffset;

	// Geometry divisions for overall vertex and triangle count
	NumSegments = NewNumSegments;
	NumRings = FMath::Max(FMath::CeilToInt((float)NumSegments * ConeAngleRadians / PI), 1);
	SegmentAngle = 2.0f * PI / (float)NumSegments;
	RingAngle = ConeAngleRadians / (float)NumRings;

	// Lighting data requires 2 extra rings for splitting normals at sharp edges
	const int32 NumVertices = bEnableLighting ? 2 + (NumRings + 3) * NumSegments : 2 + (NumRings + 1) * NumSegments;
	const int32 NumTriangles = (4 + 2 * (NumRings - 1)) * NumSegments;

	// Buffers
	Vertices.Empty();
	ConvexHull.Empty();
	Triangles.Empty();
	Normals.Empty();
	UVs.Empty();

	Vertices.Reserve(NumVertices);
	ConvexHull.Reserve(NumVertices);
	Triangles.Reserve(NumTriangles);
	if (bEnableLighting)
	{
		Normals.Reserve(NumVertices);
		UVs.Reserve(NumVertices);
	}

	// UV mapping functions

	auto NearPlaneUVMap = [TanConeAngle, NearPlane](const FVector& Position) -> FVector2D {
		const float Scale = 0.24f / (TanConeAngle * NearPlane);
		return FVector2D(0.25f + Position.Y * Scale, 0.25f + Position.Z * Scale);
	};
	auto FarPlaneUVMap = [SinConeAngle, FarRadius](const FVector& Position) -> FVector2D {
		const float Scale = 0.24f / (SinConeAngle * FarRadius);
		return FVector2D(0.75f + Position.Y * Scale, 0.25f + Position.Z * Scale);
	};
	auto ConeUVMap = [CosConeAngle, FarRadius, NearPlane](const FVector& Position) -> FVector2D {
		const float Scale = 0.24f / (CosConeAngle * FarRadius - NearPlane);
		return FVector2D(0.5f + FMath::Atan2(Position.Z, Position.Y) / (2.0f * PI) * 0.48f, 0.75f + Position.X * Scale);
	};

	// Add vertices for the near face
	const int32 CenterNearVertex = AddVertex(NearPlane, false, NearPlaneUVMap, false);
	const int32 NearVertexStart = CenterNearVertex + 1;
	AddVertexRing(TanConeAngle * NearPlane, NearPlane, PI, NearPlaneUVMap, true);
	if (bEnableLighting)
	{
		// Extra vertex ring for a sharp edge on the near plane
		AddVertexRing(TanConeAngle * NearPlane, NearPlane, 0.5f * PI + ConeAngleRadians, ConeUVMap, false);
	}

	// Add vertices for the far face
	const int32 CenterFarVertex = AddVertex(FarRadius, true, FarPlaneUVMap, true);
	const int32 FarVertexStart = CenterFarVertex + 1;
	for (int i = 0; i < NumRings; ++i)
	{
		const float Angle = (float)(i + 1) * RingAngle;
		AddVertexRing(FMath::Sin(Angle) * FarRadius, FMath::Cos(Angle) * FarRadius, Angle, FarPlaneUVMap, true);
	}
	if (bEnableLighting)
	{
		// Extra vertex ring for a sharp edge on the far plane
		AddVertexRing(SinConeAngle * FarRadius, FMath::Cos(ConeAngleRadians) * FarRadius, 0.5f * PI + ConeAngleRadians, ConeUVMap, false);
	}

	// Simple triangle fan for the near face
	AddTriangleFan(CenterNearVertex, NearVertexStart, false);
	// Concentric rings for the far face
	AddTriangleFan(CenterFarVertex, FarVertexStart, true);
	for (int i = 0; i < NumRings - 1; ++i)
	{
		AddTriangleRing(FarVertexStart + i * NumSegments, FarVertexStart + (i + 1) * NumSegments);
	}
	// Cone surface between near and far face
	if (bEnableLighting)
	{
		// Connect extra rings which have normals of the cone surface
		AddTriangleRing(FarVertexStart + NumRings * NumSegments, NearVertexStart + NumSegments);
	}
	else
	{
		// Use same vertices as the near/far planes, no normal splitting needed
		AddTriangleRing(FarVertexStart + (NumRings - 1) * NumSegments, NearVertexStart);
	}
}

void FUxtHandProximityMeshData::UpdateMesh(UProceduralMeshComponent* Mesh, int32 Section) const
{
	Mesh->CreateMeshSection(Section, Vertices, Triangles, Normals, UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), false);
	Mesh->AddCollisionConvexMesh(ConvexHull);
}

// Add a single vertex on the forward axis.
template <typename UVMapFunc>
int32 FUxtHandProximityMeshData::AddVertex(float Offset, bool FaceForward, UVMapFunc UVMap, bool bAddToConvexHull)
{
	const int32 Index = Vertices.Num();

	FVector Vertex(Offset, 0, 0);
	Vertices.Add(Vertex);
	if (bAddToConvexHull)
	{
		ConvexHull.Add(Vertex);
	}
	if (bEnableLighting)
	{
		Normals.Add(FaceForward ? FVector::ForwardVector : FVector::BackwardVector);
		UVs.Add(UVMap(Vertex));
	}

	return Index;
};

// Create a ring of vertices around the forward axis.
template <typename UVMapFunc>
int32 FUxtHandProximityMeshData::AddVertexRing(float Radius, float Offset, float AngleFromForward, UVMapFunc UVMap, bool bAddToConvexHull)
{
	const float FwdComp = FMath::Cos(AngleFromForward);
	const float LatComp = FMath::Sin(AngleFromForward);
	const int32 StartIndex = Vertices.Num();

	float Angle = 0.0f;
	for (int32 i = 0; i < NumSegments; ++i)
	{
		FVector Vertex(Offset, Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
		Vertices.Add(Vertex);
		if (bAddToConvexHull)
		{
			ConvexHull.Add(Vertex);
		}
		if (bEnableLighting)
		{
			Normals.Add(FVector(FwdComp, LatComp * FMath::Cos(Angle), LatComp * FMath::Sin(Angle)));
			UVs.Add(UVMap(Vertex));
		}

		Angle += SegmentAngle;
	}

	return StartIndex;
};

// Connect a center vertex and a vertex ring with a triangle fan.
void FUxtHandProximityMeshData::AddTriangleRing(int32 RingStartA, int32 RingStartB)
{
	for (int32 i = 0; i < NumSegments; ++i)
	{
		const int32 j = i < NumSegments - 1 ? i + 1 : 0;
		Triangles.Add(RingStartA + i);
		Triangles.Add(RingStartA + j);
		Triangles.Add(RingStartB + i);
		Triangles.Add(RingStartB + j);
		Triangles.Add(RingStartB + i);
		Triangles.Add(RingStartA + j);
	}
};

// Connect two vertex rings with a closed triangle strip.
// FaceForward determines the winding and thus normal orientation.
void FUxtHandProximityMeshData::AddTriangleFan(int32 Center, int32 RingStart, bool FaceForward)
{
	for (int32 i = 0; i < NumSegments; ++i)
	{
		const int32 j = i < NumSegments - 1 ? i + 1 : 0;
		Triangles.Add(Center);
		if (FaceForward)
		{
			Triangles.Add(RingStart + j);
			Triangles.Add(RingStart + i);
		}
		else
		{
			Triangles.Add(RingStart + i);
			Triangles.Add(RingStart + j);
		}
	}
};
