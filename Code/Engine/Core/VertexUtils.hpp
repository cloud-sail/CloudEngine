#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>
#include <string>

//-----------------------------------------------------------------------------------------------
struct FloatRange;
struct AABB2;
struct AABB3;
struct Capsule2;
struct LineSegment2;
struct OBB2;
struct OBB3;
struct Plane3;
struct Triangle2;
struct Vec2;
struct Vec3;
struct Vertex_PCU;
struct Vertex_PCUTBN;
struct Rgba8;
struct Mat44;
struct IntVec2;

//-----------------------------------------------------------------------------------------------
void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY,float rotationDegreesAboutZ, Vec2 const& translationXY);
void TransformVertexArray3D(std::vector<Vertex_PCU>& verts, Mat44 const& transform);
void TransformVertexArray3D(std::vector<Vertex_PCUTBN>& verts, Mat44 const& transform, bool changePosition = true, bool changeTBN = true);
//-----------------------------------------------------------------------------------------------
AABB2 GetVertexBounds2D(std::vector<Vertex_PCU> const& verts);


//-----------------------------------------------------------------------------------------------
void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 const& color);
void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color);
void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& color, int sideNum = 32);
void AddVertsForGradientDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& innerColor, Rgba8 const& outerColor, int sideNum = 32);
void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& color, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& bounds, Rgba8 const& color);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& startColor, Rgba8 const& endColor);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 const& color);
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 const& startColor, Rgba8 const& endColor);
void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Vec2 const& triCCW0, Vec2 const& triCCW1, Vec2 const& triCCW2, Rgba8 const& color);
void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Triangle2 const& triangle, Rgba8 const& color);
void AddVertsForRing2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color, int sideNum = 32);
void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 const& tailPos, Vec2 const& tipPos, float arrowSize, float lineThickness, Rgba8 const& color);
void AddVertsForQuad2D(std::vector<Vertex_PCU>& verts, 
	Vec2 const& bottomLeft, Vec2 const& bottomRight, Vec2 const& topRight, Vec2 const& topLeft,
	Rgba8 const& color, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad2D(std::vector<Vertex_PCU>& verts,
	Vec2 const& bottomLeft, Vec2 const& bottomRight, Vec2 const& topRight, Vec2 const& topLeft,
	Rgba8 const& color, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3);

//-----------------------------------------------------------------------------------------------
void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, 
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, 
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes, 
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, Rgba8 const& color = Rgba8::OPAQUE_WHITE);
void AddVertsForQuad3D(std::vector<Vertex_PCUTBN>& verts,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForQuad3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForRoundedQuad3D(std::vector<Vertex_PCUTBN>& verts,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForRoundedQuad3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, 
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForAABB3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	AABB3 const& bounds, Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

void AddVertsForSphere3D(std::vector<Vertex_PCU>& verts, Vec3 const& center, float radius, 
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE,
	int numSlices = 32, int numStacks = 16);
void AddVertsForUVSphere3D(std::vector<Vertex_PCU>& verts, Vec3 const& center, float radius,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE,
	int numSlices = 32, int numStacks = 16);
void AddVertsForSphere3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, 
	Vec3 const& center, float radius,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE,
	int numSlices = 32, int numStacks = 16);


void AddVertsForCylinder3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& bottomCenter, Vec3 const& topCenter, float radius,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);
void AddVertsForCylinder3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	Vec3 const& bottomCenter, Vec3 const& topCenter, float radius,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);

void AddVertsForCylinderZ3D(std::vector<Vertex_PCU>& verts, 
	Vec2 const& centerXY, FloatRange const& minMaxZ, float radius, int numSlices = 32,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);
void AddVertsForCylinderZ3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes,
	Vec2 const& centerXY, FloatRange const& minMaxZ, float radius, int numSlices = 32,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

void AddVertsForCone3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& start, Vec3 const& end, float radius,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, AABB2 const& UVs = AABB2::ZERO_TO_ONE, int numSlices = 32);

void AddVertsForArrow3D(std::vector<Vertex_PCU>& verts,
	Vec3 const& start, Vec3 const& end, float radius,
	Rgba8 const& color = Rgba8::OPAQUE_WHITE, int numSlices = 32);


void AddVertsForOBB3(std::vector<Vertex_PCU>& verts, OBB3 const& bounds, Rgba8 const& color, AABB2 const& UVs = AABB2::ZERO_TO_ONE);

void AddVertsForPenumbra3D(std::vector<Vertex_PCU>& verts, Vec3 const& position, Vec3 const& fwdNormal, float radius, float penumbraDot, Rgba8 const& color = Rgba8::OPAQUE_WHITE, int numSlices = 16);

//-----------------------------------------------------------------------------------------------
void AddVertsForGridXY(std::vector<Vertex_PCU>& verts, IntVec2 dimensions);


void AddVertsForGridPlane3D(std::vector<Vertex_PCU>& verts, Plane3 const& plane,
	Vec3 const& midPerpdicularPoint = Vec3::ZERO, Vec2 const& gridSize = Vec2(100.f, 100.f), Vec2 const& cellSize = Vec2(1.f, 1.f),
	float gridThickness = 0.025f, Rgba8 xAxisColor = Rgba8::RED, Rgba8 yAxisColor = Rgba8::GREEN);
