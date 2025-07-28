#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Triangle2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/IntVec2.hpp"


void TransformVertexArrayXY3D(int numVerts, Vertex_PCU* verts, float uniformScaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY)
{
	for (int vertIndex = 0; vertIndex < numVerts; ++vertIndex)
	{
		TransformPositionXY3D(verts[vertIndex].m_position, uniformScaleXY, rotationDegreesAboutZ, translationXY);
	}
}

void TransformVertexArray3D(std::vector<Vertex_PCU>& verts, Mat44 const& transform)
{
	int vertsSize = (int)verts.size();
	for (int index = 0; index < vertsSize; ++index)
	{
		verts[index].m_position = transform.TransformPosition3D(verts[index].m_position);
	}
}



void TransformVertexArray3D(std::vector<Vertex_PCUTBN>& verts, Mat44 const& transform, bool changePosition /*= true*/, bool changeTBN /*= true*/)
{
	int vertsSize = (int)verts.size();
	for (int index = 0; index < vertsSize; ++index)
	{
		if (changePosition)
		{
			verts[index].m_position = transform.TransformPosition3D(verts[index].m_position);

		}
		if (changeTBN)
		{
			verts[index].m_tangent = transform.TransformPosition3D(verts[index].m_tangent);
			verts[index].m_bitangent = transform.TransformPosition3D(verts[index].m_bitangent);
			verts[index].m_normal = transform.TransformPosition3D(verts[index].m_normal);
		}
	}
}

AABB2 GetVertexBounds2D(std::vector<Vertex_PCU> const& verts)
{
	int vertsSize = (int)verts.size();
	if (vertsSize == 0)
	{
		return AABB2();
	}

	AABB2 box(verts[0].m_position.x, verts[0].m_position.y, verts[0].m_position.x, verts[0].m_position.y);
	for (int index = 1; index < vertsSize; ++index)
	{
		box.StretchToIncludePoint(Vec2(verts[index].m_position.x, verts[index].m_position.y));
	}
	return box;
}

void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 const& color)
{
	AddVertsForCapsule2D(verts, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius, color);
}

void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius, Rgba8 const& color)
{
	constexpr int NUM_SIDES = 16; // Semi-Circle
	constexpr float DEGREES_PER_SIDE = 180.f / static_cast<float>(NUM_SIDES);

	Vec2 left = (boneEnd - boneStart).GetRotated90Degrees();
	left.SetLength(radius);

	Vec2 startLeft = boneStart + left;
	Vec2 startRight = boneStart - left;
	Vec2 endLeft = boneEnd + left;
	Vec2 endRight = boneEnd - left;

	// Push Back Capsule Body
	verts.emplace_back(Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), color, Vec2::ZERO));

	verts.emplace_back(Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), color, Vec2::ZERO));

	// Push Back Capsule Start Semi-Circle

	Vec2 currentSide = left;
	for (int i = 0; i < NUM_SIDES; ++i)
	{
		verts.emplace_back(Vertex_PCU(Vec3(boneStart.x, boneStart.y, 0.f), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(boneStart.x + currentSide.x, boneStart.y + currentSide.y, 0.f), color, Vec2::ZERO));
		currentSide.RotateDegrees(DEGREES_PER_SIDE);
		verts.emplace_back(Vertex_PCU(Vec3(boneStart.x + currentSide.x, boneStart.y + currentSide.y, 0.f), color, Vec2::ZERO));
	}

	// Push Back Capsule End Semi-Circle
	currentSide = -left;
	for (int i = 0; i < NUM_SIDES; ++i)
	{
		verts.emplace_back(Vertex_PCU(Vec3(boneEnd.x, boneEnd.y, 0.f), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(boneEnd.x + currentSide.x, boneEnd.y + currentSide.y, 0.f), color, Vec2::ZERO));
		currentSide.RotateDegrees(DEGREES_PER_SIDE);
		verts.emplace_back(Vertex_PCU(Vec3(boneEnd.x + currentSide.x, boneEnd.y + currentSide.y, 0.f), color, Vec2::ZERO));
	}
}

//void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& color)
//{
//	constexpr int NUM_SIDES = 32;
//	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);
//
//	for (int i = 0; i < NUM_SIDES; ++i)
//	{
//		float firstDegrees = i * DEGREES_PER_SIDE;
//		float secondDegrees = (i + 1) * DEGREES_PER_SIDE;
//
//		Vec3 firstPoint = Vec3(center.x + radius * CosDegrees(firstDegrees), center.y + radius * SinDegrees(firstDegrees), 0.f);
//		Vec3 secondPoint = Vec3(center.x + radius * CosDegrees(secondDegrees), center.y + radius * SinDegrees(secondDegrees), 0.f);
//
//		verts.emplace_back(Vertex_PCU(Vec3(center.x, center.y, 0.f), color, Vec2::ZERO));
//		verts.emplace_back(Vertex_PCU(firstPoint, color, Vec2::ZERO));
//		verts.emplace_back(Vertex_PCU(secondPoint, color, Vec2::ZERO));
//	}
//}

void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& color, int sideNum /*= 32*/)
{
	const float DEGREES_PER_SIDE = 360.f / static_cast<float>(sideNum);

	for (int i = 0; i < sideNum; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SIDE;
		float secondDegrees = (i + 1) * DEGREES_PER_SIDE;

		Vec3 firstPoint = Vec3(center.x + radius * CosDegrees(firstDegrees), center.y + radius * SinDegrees(firstDegrees), 0.f);
		Vec3 secondPoint = Vec3(center.x + radius * CosDegrees(secondDegrees), center.y + radius * SinDegrees(secondDegrees), 0.f);

		verts.emplace_back(Vertex_PCU(Vec3(center.x, center.y, 0.f), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(firstPoint, color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(secondPoint, color, Vec2::ZERO));
	}
}


void AddVertsForGradientDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, Rgba8 const& innerColor, Rgba8 const& outerColor, int sideNum /*= 32*/)
{
	const float DEGREES_PER_SIDE = 360.f / static_cast<float>(sideNum);

	for (int i = 0; i < sideNum; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SIDE;
		float secondDegrees = (i + 1) * DEGREES_PER_SIDE;

		Vec3 firstPoint = Vec3(center.x + radius * CosDegrees(firstDegrees), center.y + radius * SinDegrees(firstDegrees), 0.f);
		Vec3 secondPoint = Vec3(center.x + radius * CosDegrees(secondDegrees), center.y + radius * SinDegrees(secondDegrees), 0.f);

		verts.emplace_back(Vertex_PCU(Vec3(center.x, center.y, 0.f), innerColor, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(firstPoint, outerColor, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(secondPoint, outerColor, Vec2::ZERO));
	}
}

void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, AABB2 const& bounds, Rgba8 const& color, AABB2 const& UVs)
{
	Vec2 topLeft(bounds.m_mins.x, bounds.m_maxs.y);
	Vec2 topRight(bounds.m_maxs.x, bounds.m_maxs.y);
	Vec2 bottomLeft(bounds.m_mins.x, bounds.m_mins.y);
	Vec2 bottomRight(bounds.m_maxs.x, bounds.m_mins.y);

	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, UVs.m_mins));
	verts.emplace_back(Vertex_PCU(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(UVs.m_maxs.x, UVs.m_mins.y)));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, UVs.m_maxs));

	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, UVs.m_mins));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, UVs.m_maxs));
	verts.emplace_back(Vertex_PCU(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(UVs.m_mins.x, UVs.m_maxs.y)));
}

void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, OBB2 const& bounds, Rgba8 const& color)
{
	Vec2 cornerPoints[4];
	bounds.GetCornerPoints(cornerPoints);

	verts.emplace_back(Vertex_PCU(Vec3(cornerPoints[0].x, cornerPoints[0].y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(cornerPoints[1].x, cornerPoints[1].y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(cornerPoints[2].x, cornerPoints[2].y, 0.f), color, Vec2::ZERO));

	verts.emplace_back(Vertex_PCU(Vec3(cornerPoints[0].x, cornerPoints[0].y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(cornerPoints[2].x, cornerPoints[2].y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(cornerPoints[3].x, cornerPoints[3].y, 0.f), color, Vec2::ZERO));
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color)
{
	Vec2 forward = (end - start).GetNormalized();
	Vec2 left = forward.GetRotated90Degrees();
	float h = thickness * 0.5f;

	Vec2 startLeft = start - forward * h + left * h;
	Vec2 startRight = start - forward * h - left * h;
	Vec2 endLeft = end + forward * h + left * h;
	Vec2 endRight = end + forward * h - left * h;

	verts.emplace_back(Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), color, Vec2::ZERO));

	verts.emplace_back(Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), color, Vec2::ZERO));
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& startColor, Rgba8 const& endColor)
{
	Vec2 forward = (end - start).GetNormalized();
	Vec2 left = forward.GetRotated90Degrees();
	float h = thickness * 0.5f;

	Vec2 startLeft = start - forward * h + left * h;
	Vec2 startRight = start - forward * h - left * h;
	Vec2 endLeft = end + forward * h + left * h;
	Vec2 endRight = end + forward * h - left * h;

	verts.emplace_back(Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), startColor, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(startRight.x, startRight.y, 0.f), startColor, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), endColor, Vec2::ZERO));

	verts.emplace_back(Vertex_PCU(Vec3(startLeft.x, startLeft.y, 0.f), startColor, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endRight.x, endRight.y, 0.f), endColor, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(endLeft.x, endLeft.y, 0.f), endColor, Vec2::ZERO));
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 const& color)
{
	AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, thickness, color);
}

void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 const& startColor, Rgba8 const& endColor)
{
	AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, thickness, startColor, endColor);
}

void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Vec2 const& triCCW0, Vec2 const& triCCW1, Vec2 const& triCCW2, Rgba8 const& color)
{
	verts.emplace_back(Vertex_PCU(Vec3(triCCW0.x, triCCW0.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(triCCW1.x, triCCW1.y, 0.f), color, Vec2::ZERO));
	verts.emplace_back(Vertex_PCU(Vec3(triCCW2.x, triCCW2.y, 0.f), color, Vec2::ZERO));
}

void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Triangle2 const& triangle, Rgba8 const& color)
{
	AddVertsForTriangle2D(verts, triangle.m_pointsCounterClockwise[0], triangle.m_pointsCounterClockwise[1], triangle.m_pointsCounterClockwise[2], color);
}

void AddVertsForRing2D(std::vector<Vertex_PCU>& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color, int sideNum)
{
	float DEGREES_PER_SIDE = 360.f / static_cast<float>(sideNum);

	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;

	for (int sideIndex = 0; sideIndex < sideNum; ++sideIndex)
	{
		// Compute angle-related terms
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideIndex + 1);

		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);

		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		// Compute inner & outer positions
		Vec3 innerStartPos = Vec3(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos = Vec3(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 innerEndPos = Vec3(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);
		Vec3 outerEndPos = Vec3(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);

		// Trapezoid is made of two triangles: ABC and DEF
		// A is inner end; B is inner start; C is outer start
		// D is inner end; E is outer start; F is outer end
		verts.emplace_back(Vertex_PCU(Vec3(innerEndPos), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(innerStartPos), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(outerStartPos), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(innerEndPos), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(outerStartPos), color, Vec2::ZERO));
		verts.emplace_back(Vertex_PCU(Vec3(outerEndPos), color, Vec2::ZERO));
	}
}

void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 const& tailPos, Vec2 const& tipPos, float arrowSize, float lineThickness, Rgba8 const& color)
{
	if (tailPos == tipPos)
	{
		return;
	}
	AddVertsForLineSegment2D(verts, tailPos, tipPos, lineThickness, color);

	Vec2 disp = tailPos - tipPos;
	Vec2 leftArrowTipDisp = disp.GetRotatedDegrees(-45.f);
	leftArrowTipDisp.SetLength(arrowSize);
	Vec2 rightArrowTipDisp = leftArrowTipDisp.GetRotated90Degrees();

	AddVertsForLineSegment2D(verts, tipPos, tipPos + leftArrowTipDisp, lineThickness, color);
	AddVertsForLineSegment2D(verts, tipPos, tipPos + rightArrowTipDisp, lineThickness, color);
}

void AddVertsForQuad2D(std::vector<Vertex_PCU>& verts, Vec2 const& bottomLeft, Vec2 const& bottomRight, Vec2 const& topRight, Vec2 const& topLeft, Rgba8 const& color, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, UVs.m_mins));
	verts.emplace_back(Vertex_PCU(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(UVs.m_maxs.x, UVs.m_mins.y)));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, UVs.m_maxs));

	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, UVs.m_mins));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, UVs.m_maxs));
	verts.emplace_back(Vertex_PCU(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(UVs.m_mins.x, UVs.m_maxs.y)));
}


void AddVertsForQuad2D(std::vector<Vertex_PCU>& verts, Vec2 const& bottomLeft, Vec2 const& bottomRight, Vec2 const& topRight, Vec2 const& topLeft, Rgba8 const& color, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3)
{
	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, uv0));
	verts.emplace_back(Vertex_PCU(Vec3(bottomRight.x, bottomRight.y, 0.f), color, uv1));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, uv2));

	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, uv0));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, uv2));
	verts.emplace_back(Vertex_PCU(Vec3(topLeft.x, topLeft.y, 0.f), color, uv3));
}

void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, bottomLeft.z), color, UVs.m_mins));
	verts.emplace_back(Vertex_PCU(Vec3(bottomRight.x, bottomRight.y, bottomRight.z), color, Vec2(UVs.m_maxs.x, UVs.m_mins.y)));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, topRight.z), color, UVs.m_maxs));

	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, bottomLeft.z), color, UVs.m_mins));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, topRight.z), color, UVs.m_maxs));
	verts.emplace_back(Vertex_PCU(Vec3(topLeft.x, topLeft.y, topLeft.z), color, Vec2(UVs.m_mins.x, UVs.m_maxs.y)));
}

void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Vec2 uv0, Vec2 uv1, Vec2 uv2, Vec2 uv3, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/)
{
	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, bottomLeft.z), color, uv0));
	verts.emplace_back(Vertex_PCU(Vec3(bottomRight.x, bottomRight.y, bottomRight.z), color, uv1));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, topRight.z), color, uv2));

	verts.emplace_back(Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, bottomLeft.z), color, uv0));
	verts.emplace_back(Vertex_PCU(Vec3(topRight.x, topRight.y, topRight.z), color, uv2));
	verts.emplace_back(Vertex_PCU(Vec3(topLeft.x, topLeft.y, topLeft.z), color, uv3));
}

void AddVertsForQuad3D(std::vector<Vertex_PCU>& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	unsigned int startIndex = static_cast<unsigned int>(verts.size());

	Vec2 uv0 = UVs.m_mins;
	Vec2 uv1 = Vec2(UVs.m_maxs.x, UVs.m_mins.y);
	Vec2 uv2 = UVs.m_maxs;
	Vec2 uv3 = Vec2(UVs.m_mins.x, UVs.m_maxs.y);

	verts.push_back(Vertex_PCU(bottomLeft, color, uv0));
	verts.push_back(Vertex_PCU(bottomRight, color, uv1));
	verts.push_back(Vertex_PCU(topRight, color, uv2));
	verts.push_back(Vertex_PCU(topLeft, color, uv3));

	indexes.push_back(startIndex);
	indexes.push_back(startIndex + 1);
	indexes.push_back(startIndex + 2);

	indexes.push_back(startIndex);
	indexes.push_back(startIndex + 2);
	indexes.push_back(startIndex + 3);
}

void AddVertsForQuad3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	unsigned int startIndex = static_cast<unsigned int>(verts.size());

	Vec2 uv0 = UVs.m_mins;
	Vec2 uv1 = Vec2(UVs.m_maxs.x, UVs.m_mins.y);
	Vec2 uv2 = UVs.m_maxs;
	Vec2 uv3 = Vec2(UVs.m_mins.x, UVs.m_maxs.y);

	Vec3 tangent = (bottomRight - bottomLeft).GetNormalized();
	Vec3 bitangent = (topLeft - bottomLeft).GetNormalized();
	Vec3 normal = CrossProduct3D(bottomRight - bottomLeft, topLeft - bottomLeft).GetNormalized();

	verts.push_back(Vertex_PCUTBN(bottomLeft, color, uv0, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(bottomRight, color, uv1, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topRight, color, uv2, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topLeft, color, uv3, tangent, bitangent, normal));

	indexes.push_back(startIndex);
	indexes.push_back(startIndex + 1);
	indexes.push_back(startIndex + 2);

	indexes.push_back(startIndex);
	indexes.push_back(startIndex + 2);
	indexes.push_back(startIndex + 3);
}

void AddVertsForQuad3D(std::vector<Vertex_PCUTBN>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	Vec2 uv0 = UVs.m_mins;
	Vec2 uv1 = Vec2(UVs.m_maxs.x, UVs.m_mins.y);
	Vec2 uv2 = UVs.m_maxs;
	Vec2 uv3 = Vec2(UVs.m_mins.x, UVs.m_maxs.y);

	Vec3 tangent = (bottomRight - bottomLeft).GetNormalized();
	Vec3 bitangent = (topLeft - bottomLeft).GetNormalized();
	Vec3 normal = CrossProduct3D(bottomRight - bottomLeft, topLeft - bottomLeft).GetNormalized();

	verts.push_back(Vertex_PCUTBN(bottomLeft, color, uv0, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(bottomRight, color, uv1, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topRight, color, uv2, tangent, bitangent, normal));

	verts.push_back(Vertex_PCUTBN(bottomLeft, color, uv0, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topRight, color, uv2, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(topLeft, color, uv3, tangent, bitangent, normal));
}

void AddVertsForRoundedQuad3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	unsigned int startIndex = static_cast<unsigned int>(verts.size());

	// <-A C E->
	// <-B D F->
	Vec3 A = topLeft;
	Vec3 B = bottomLeft;
	Vec3 E = topRight;
	Vec3 F = bottomRight;
	Vec3 C = 0.5f * (A + E);
	Vec3 D = 0.5f * (B + F);

	float uvCenterX = 0.5f * (UVs.m_mins.x + UVs.m_maxs.x);
	Vec2 uvA = Vec2(UVs.m_mins.x, UVs.m_maxs.y);
	Vec2 uvB = UVs.m_mins;
	Vec2 uvC = Vec2(uvCenterX, UVs.m_maxs.y);
	Vec2 uvD = Vec2(uvCenterX, UVs.m_mins.y);
	Vec2 uvE = UVs.m_maxs;
	Vec2 uvF = Vec2(UVs.m_maxs.x, UVs.m_mins.y);

	Vec3 normal = CrossProduct3D(bottomRight - bottomLeft, topLeft - bottomLeft).GetNormalized();
	Vec3 uDirection = (bottomRight - bottomLeft).GetNormalized();

	Vec3 tangent = Vec3::ZERO;
	Vec3 bitangent = Vec3::ZERO;

	verts.push_back(Vertex_PCUTBN(A, color, uvA, tangent, bitangent, -uDirection));
	verts.push_back(Vertex_PCUTBN(B, color, uvB, tangent, bitangent, -uDirection));
	verts.push_back(Vertex_PCUTBN(C, color, uvC, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(D, color, uvD, tangent, bitangent, normal));
	verts.push_back(Vertex_PCUTBN(E, color, uvE, tangent, bitangent, uDirection));
	verts.push_back(Vertex_PCUTBN(F, color, uvF, tangent, bitangent, uDirection));

	unsigned int indexA = startIndex;
	unsigned int indexB = startIndex + 1;
	unsigned int indexC = startIndex + 2;
	unsigned int indexD = startIndex + 3;
	unsigned int indexE = startIndex + 4;
	unsigned int indexF = startIndex + 5;

	indexes.push_back(indexB);
	indexes.push_back(indexD);
	indexes.push_back(indexC);

	indexes.push_back(indexB);
	indexes.push_back(indexC);
	indexes.push_back(indexA);

	indexes.push_back(indexD);
	indexes.push_back(indexF);
	indexes.push_back(indexE);

	indexes.push_back(indexD);
	indexes.push_back(indexE);
	indexes.push_back(indexC);
}

void AddVertsForRoundedQuad3D(std::vector<Vertex_PCUTBN>& verts, Vec3 const& bottomLeft, Vec3 const& bottomRight, Vec3 const& topRight, Vec3 const& topLeft, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	// <-A C E->
	// <-B D F->
	Vec3 A = topLeft;
	Vec3 B = bottomLeft;
	Vec3 E = topRight;
	Vec3 F = bottomRight;
	Vec3 C = 0.5f * (A + E);
	Vec3 D = 0.5f * (B + F);

	float uvCenterX = 0.5f * (UVs.m_mins.x + UVs.m_maxs.x);
	Vec2 uvA = Vec2(UVs.m_mins.x, UVs.m_maxs.y);
	Vec2 uvB = UVs.m_mins;
	Vec2 uvC = Vec2(uvCenterX, UVs.m_maxs.y);
	Vec2 uvD = Vec2(uvCenterX, UVs.m_mins.y);
	Vec2 uvE = UVs.m_maxs;
	Vec2 uvF = Vec2(UVs.m_maxs.x, UVs.m_mins.y);

	Vec3 normal = CrossProduct3D(bottomRight - bottomLeft, topLeft - bottomLeft).GetNormalized();
	Vec3 uDirection = (bottomRight - bottomLeft).GetNormalized();

	Vec3 tangent = Vec3::ZERO;
	Vec3 bitangent = Vec3::ZERO;

	Vertex_PCUTBN vertexA(A, color, uvA, tangent, bitangent, -uDirection);
	Vertex_PCUTBN vertexB(B, color, uvB, tangent, bitangent, -uDirection);
	Vertex_PCUTBN vertexC(C, color, uvC, tangent, bitangent, normal);
	Vertex_PCUTBN vertexD(D, color, uvD, tangent, bitangent, normal);
	Vertex_PCUTBN vertexE(E, color, uvE, tangent, bitangent, uDirection);
	Vertex_PCUTBN vertexF(F, color, uvF, tangent, bitangent, uDirection);

	verts.push_back(vertexB);
	verts.push_back(vertexD);
	verts.push_back(vertexC);

	verts.push_back(vertexB);
	verts.push_back(vertexC);
	verts.push_back(vertexA);

	verts.push_back(vertexD);
	verts.push_back(vertexF);
	verts.push_back(vertexE);

	verts.push_back(vertexD);
	verts.push_back(vertexE);
	verts.push_back(vertexC);
}

void AddVertsForAABB3D(std::vector<Vertex_PCU>& verts, AABB3 const& bounds, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	float minX = bounds.m_mins.x;
	float minY = bounds.m_mins.y;
	float minZ = bounds.m_mins.z;
	float maxX = bounds.m_maxs.x;
	float maxY = bounds.m_maxs.y;
	float maxZ = bounds.m_maxs.z;

	// p-max n-min
	Vec3 nnn(minX, minY, minZ);
	Vec3 nnp(minX, minY, maxZ);
	Vec3 npn(minX, maxY, minZ);
	Vec3 npp(minX, maxY, maxZ);
	Vec3 pnn(maxX, minY, minZ);
	Vec3 pnp(maxX, minY, maxZ);
	Vec3 ppn(maxX, maxY, minZ);
	Vec3 ppp(maxX, maxY, maxZ);

	AddVertsForQuad3D(verts, pnn, ppn, ppp, pnp, color, UVs); // +x
	AddVertsForQuad3D(verts, npn, nnn, nnp, npp, color, UVs); // -x
	AddVertsForQuad3D(verts, ppn, npn, npp, ppp, color, UVs); // +y
	AddVertsForQuad3D(verts, nnn, pnn, pnp, nnp, color, UVs); // -y
	AddVertsForQuad3D(verts, nnp, pnp, ppp, npp, color, UVs); // +z
	AddVertsForQuad3D(verts, npn, ppn, pnn, nnn, color, UVs); // -z
}

void AddVertsForAABB3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, AABB3 const& bounds, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	float minX = bounds.m_mins.x;
	float minY = bounds.m_mins.y;
	float minZ = bounds.m_mins.z;
	float maxX = bounds.m_maxs.x;
	float maxY = bounds.m_maxs.y;
	float maxZ = bounds.m_maxs.z;

	// p-max n-min
	Vec3 nnn(minX, minY, minZ);
	Vec3 nnp(minX, minY, maxZ);
	Vec3 npn(minX, maxY, minZ);
	Vec3 npp(minX, maxY, maxZ);
	Vec3 pnn(maxX, minY, minZ);
	Vec3 pnp(maxX, minY, maxZ);
	Vec3 ppn(maxX, maxY, minZ);
	Vec3 ppp(maxX, maxY, maxZ);

	AddVertsForQuad3D(verts, indexes, pnn, ppn, ppp, pnp, color, UVs); // +x
	AddVertsForQuad3D(verts, indexes, npn, nnn, nnp, npp, color, UVs); // -x
	AddVertsForQuad3D(verts, indexes, ppn, npn, npp, ppp, color, UVs); // +y
	AddVertsForQuad3D(verts, indexes, nnn, pnn, pnp, nnp, color, UVs); // -y
	AddVertsForQuad3D(verts, indexes, nnp, pnp, ppp, npp, color, UVs); // +z
	AddVertsForQuad3D(verts, indexes, npn, ppn, pnn, nnn, color, UVs); // -z
}

void AddVertsForSphere3D(std::vector<Vertex_PCU>& verts, Vec3 const& center, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/, int numStacks /*= 16*/)
{
	// simple version Change to slides version later
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);
	float const DEGREES_PER_STACK = 180.f / static_cast<float>(numStacks);

	float const UV_STEP_X = UVs.GetDimensions().x / static_cast<float>(numSlices);
	float const UV_STEP_Y = UVs.GetDimensions().y / static_cast<float>(numStacks);

	for (int stackIndex = 0; stackIndex < numStacks; stackIndex++)
	{
		for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++)
		{
			float yawDegrees = static_cast<float>(sliceIndex) * DEGREES_PER_SLICE;
			float pitchDegrees = 90.f - static_cast<float>(stackIndex) * DEGREES_PER_STACK;
			Vec3 BL = center + Vec3::MakeFromPolarDegrees(pitchDegrees, yawDegrees, radius);
			Vec3 BR = center + Vec3::MakeFromPolarDegrees(pitchDegrees, yawDegrees + DEGREES_PER_SLICE, radius);
			Vec3 TL = center + Vec3::MakeFromPolarDegrees(pitchDegrees - DEGREES_PER_STACK, yawDegrees, radius);
			Vec3 TR = center + Vec3::MakeFromPolarDegrees(pitchDegrees - DEGREES_PER_STACK, yawDegrees + DEGREES_PER_SLICE, radius);
			
			AABB2 quadUVs(static_cast<float>(sliceIndex) * UV_STEP_X,
				static_cast<float>(stackIndex) * UV_STEP_Y,
				static_cast<float>(sliceIndex+1) * UV_STEP_X,
				static_cast<float>(stackIndex+1) * UV_STEP_Y);
			
			AddVertsForQuad3D(verts, BL, BR, TR, TL, color, quadUVs);
		}
	}
}

void AddVertsForSphere3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec3 const& center, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/, int numStacks /*= 16*/)
{
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);
	float const DEGREES_PER_STACK = 180.f / static_cast<float>(numStacks);
	unsigned int const startIndex = static_cast<unsigned int>(verts.size());
	int const STRIDE = (numSlices + 1);


	for (int stackIndex = 0; stackIndex <= numStacks; stackIndex++)
	{
		for (int sliceIndex = 0; sliceIndex <= numSlices; sliceIndex++)
		{
			float yawDegrees = static_cast<float>(sliceIndex) * DEGREES_PER_SLICE;
			float pitchDegrees = 90.f - static_cast<float>(stackIndex) * DEGREES_PER_STACK;

			Vec3 disp = Vec3::MakeFromPolarDegrees(pitchDegrees, yawDegrees, radius);
			Vec3 pos = center + disp;
			Vec2 uv = UVs.GetPointAtUV(Vec2((float)sliceIndex / (float)numSlices, (float)stackIndex / (float)numStacks));

			Vec3 tangent = Vec3(-SinDegrees(yawDegrees), CosDegrees(yawDegrees), 0.f);
			Vec3 normal = disp.GetNormalized();
			Vec3 bitangent = CrossProduct3D(normal, tangent);

			verts.push_back(Vertex_PCUTBN(pos, color, uv, tangent, bitangent, normal));
		}
	}

	for (int stackIndex = 0; stackIndex < numStacks; stackIndex++)
	{
		for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++)
		{
			if (stackIndex == 0)
			{
				indexes.push_back(startIndex + sliceIndex);
				indexes.push_back(startIndex + sliceIndex + STRIDE + 1);
				indexes.push_back(startIndex + sliceIndex + STRIDE);
			}
			else if (stackIndex == (numStacks - 1))
			{
				int offset = (numStacks - 1) * STRIDE + sliceIndex;

				indexes.push_back(startIndex + offset);
				indexes.push_back(startIndex + offset + 1);
				indexes.push_back(startIndex + offset + STRIDE);
			}
			else
			{
				int offset = stackIndex * STRIDE + sliceIndex;

				indexes.push_back(startIndex + offset);
				indexes.push_back(startIndex + offset + 1);
				indexes.push_back(startIndex + offset + STRIDE + 1);

				indexes.push_back(startIndex + offset);
				indexes.push_back(startIndex + offset + STRIDE + 1);
				indexes.push_back(startIndex + offset + STRIDE);
			}
		}
	}
}

void AddVertsForUVSphere3D(std::vector<Vertex_PCU>& verts, Vec3 const& center, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/, int numStacks /*= 16*/)
{
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);
	float const DEGREES_PER_STACK = 180.f / static_cast<float>(numStacks);

	std::vector<Vertex_PCU> vertices;
	vertices.reserve((numStacks + 1) * (numSlices + 1));

	for (int stackIndex = 0; stackIndex <= numStacks; stackIndex++)
	{
		for (int sliceIndex = 0; sliceIndex <= numSlices; sliceIndex++)
		{
			float yawDegrees = static_cast<float>(sliceIndex) * DEGREES_PER_SLICE;
			float pitchDegrees = 90 - static_cast<float>(stackIndex) * DEGREES_PER_STACK;
			Vec3 pos = center + Vec3::MakeFromPolarDegrees(pitchDegrees, yawDegrees, radius);
			Vec2 uv = UVs.GetPointAtUV(Vec2((float)sliceIndex / (float)numSlices, (float)stackIndex / (float)numStacks));

			vertices.emplace_back(pos, color, uv);
		}
	}

	int const STRIDE = (numSlices + 1);

	for (int stackIndex = 0; stackIndex < numStacks; stackIndex++)
	{
		for (int sliceIndex = 0; sliceIndex < numSlices; sliceIndex++)
		{
			if (stackIndex == 0)
			{
				verts.emplace_back(vertices[sliceIndex]);
				verts.emplace_back(vertices[sliceIndex + STRIDE + 1]);
				verts.emplace_back(vertices[sliceIndex + STRIDE]);
			}
			else if (stackIndex == (numStacks - 1))
			{
				int offset = (numStacks - 1) * STRIDE + sliceIndex;
				verts.emplace_back(vertices[offset]);
				verts.emplace_back(vertices[offset + 1]);
				verts.emplace_back(vertices[offset + STRIDE]);
			}
			else
			{
				int offset = stackIndex * STRIDE + sliceIndex;
				verts.emplace_back(vertices[offset]);
				verts.emplace_back(vertices[offset + 1]);
				verts.emplace_back(vertices[offset + STRIDE + 1]);

				verts.emplace_back(vertices[offset]);
				verts.emplace_back(vertices[offset + STRIDE + 1]);
				verts.emplace_back(vertices[offset + STRIDE]);
			}
		}
	}
}

void AddVertsForCylinder3D(std::vector<Vertex_PCU>& verts, Vec3 const& bottomCenter, Vec3 const& topCenter, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/)
{
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);
	Mat44 localSpace = Mat44::MakeFromX(topCenter - bottomCenter);
	Vec3 iBasis = localSpace.GetIBasis3D();
	Vec3 jBasis = localSpace.GetJBasis3D();
	Vec3 kBasis = localSpace.GetKBasis3D();


	for (int i = 0; i < numSlices; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SLICE;
		float secondDegrees = (i + 1) * DEGREES_PER_SLICE;

		Vec3 firstOffset = jBasis * radius * CosDegrees(firstDegrees) + kBasis * radius * SinDegrees(firstDegrees);
		Vec3 secondOffset = jBasis * radius * CosDegrees(secondDegrees) + kBasis * radius * SinDegrees(secondDegrees);

		Vec3 BL = bottomCenter + firstOffset;
		Vec3 BR = bottomCenter + secondOffset;
		Vec3 TL = topCenter + firstOffset;
		Vec3 TR = topCenter + secondOffset;

		// Top and Bottom Circle
		verts.push_back(Vertex_PCU(bottomCenter, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BR, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BL, color, Vec2::ZERO));

		verts.push_back(Vertex_PCU(topCenter, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(TL, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(TR, color, Vec2::ZERO));

		// Side Quad
		AddVertsForQuad3D(verts, BL, BR, TR, TL, color, UVs);
	}
}

void AddVertsForCylinder3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec3 const& bottomCenter, Vec3 const& topCenter, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/)
{
	unsigned int const startIndex = static_cast<unsigned int>(verts.size());
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);

	Mat44 localSpace = Mat44::MakeFromX(topCenter - bottomCenter);
	Vec3 iBasis = localSpace.GetIBasis3D();
	Vec3 jBasis = localSpace.GetJBasis3D();
	Vec3 kBasis = localSpace.GetKBasis3D();

	Vec2 uvCenter = UVs.GetCenter();
	Vec2 halfDimensions = UVs.GetDimensions() * 0.5f;

	// Bottom and Top Center (0, 1)
	verts.push_back(Vertex_PCUTBN(bottomCenter, color, uvCenter, jBasis, -kBasis, -iBasis));
	verts.push_back(Vertex_PCUTBN(topCenter, color, uvCenter, jBasis, kBasis, iBasis));

	for (int i = 0; i <= numSlices; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SLICE;

		float firstCos = CosDegrees(firstDegrees);
		float firstSin = SinDegrees(firstDegrees);

		Vec3 firstOffset	= jBasis * radius * firstCos	+ kBasis * radius * firstSin;

		Vec3 BL = bottomCenter	+ firstOffset;
		Vec3 TL = topCenter		+ firstOffset;

		// Bottom
		verts.push_back(Vertex_PCUTBN(BL, color, uvCenter + Vec2(firstCos, -firstSin) * halfDimensions, jBasis, -kBasis, -iBasis));

		// Top
		verts.push_back(Vertex_PCUTBN(TL, color, uvCenter + Vec2(firstCos, firstSin) * halfDimensions, jBasis, kBasis, iBasis));

		// Side
		float sideIncrement = UVs.GetDimensions().x / static_cast<float>(numSlices);
		float minX = UVs.m_mins.x + sideIncrement * static_cast<float>(i);


		Vec3 firstNormal = jBasis * firstCos + kBasis * firstSin;
		Vec3 tangent = CrossProduct3D(iBasis, firstNormal);

		verts.push_back(Vertex_PCUTBN(BL, color, Vec2(minX, UVs.m_mins.y), tangent, iBasis, firstNormal));
		verts.push_back(Vertex_PCUTBN(TL, color, Vec2(minX, UVs.m_maxs.y), tangent, iBasis, firstNormal));
	}

	for (int i = 0; i < numSlices; ++i)
	{
		int offset = startIndex + 4 * i + 2;
		indexes.push_back(startIndex);	// Bottom Center
		indexes.push_back(offset + 4);	// BR
		indexes.push_back(offset);		// BL

		indexes.push_back(startIndex + 1);	// Top Center
		indexes.push_back(offset + 1);		// TL
		indexes.push_back(offset + 5);		// TR

		indexes.push_back(offset + 2);	// Quad BL
		indexes.push_back(offset + 6);	// Quad BR
		indexes.push_back(offset + 7);	// Quad TR

		indexes.push_back(offset + 2);	// Quad BL
		indexes.push_back(offset + 7);	// Quad TR
		indexes.push_back(offset + 3);	// Quad TL
	}
}

void AddVertsForCone3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/, int numSlices /*= 32*/)
{
	UNUSED(UVs);

	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);
	Mat44 localSpace = Mat44::MakeFromX(end - start);
	Vec3 iBasis = localSpace.GetIBasis3D();
	Vec3 jBasis = localSpace.GetJBasis3D();
	Vec3 kBasis = localSpace.GetKBasis3D();

	for (int i = 0; i < numSlices; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SLICE;
		float secondDegrees = (i + 1) * DEGREES_PER_SLICE;

		Vec3 firstOffset = jBasis * radius * CosDegrees(firstDegrees) + kBasis * radius * SinDegrees(firstDegrees);
		Vec3 secondOffset = jBasis * radius * CosDegrees(secondDegrees) + kBasis * radius * SinDegrees(secondDegrees);

		Vec3 BL = start + firstOffset;
		Vec3 BR = start + secondOffset;

		// Bottom Circle
		verts.push_back(Vertex_PCU(start, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BR, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BL, color, Vec2::ZERO));

		// Cone Side
		verts.push_back(Vertex_PCU(end, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BL, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BR, color, Vec2::ZERO));
	}
}

void AddVertsForArrow3D(std::vector<Vertex_PCU>& verts, Vec3 const& start, Vec3 const& end, float radius, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, int numSlices /*= 32*/)
{
	Vec3 arrowVec = end - start;
	float arrowLength = arrowVec.GetLength();
	Vec3 direction = arrowVec.GetNormalized();

	float coneRadius = 2.f * radius;
	float coneHeight = 3.236f * coneRadius;
	if (coneHeight > (arrowLength * 0.382f))
	{
		coneHeight = arrowLength * 0.382f;
	}

	float cylinderHeight = arrowLength - coneHeight;

	Vec3 cylinderStart = start;
	Vec3 cylinderEnd = start + direction * cylinderHeight;

	Vec3 coneStart = cylinderEnd;
	Vec3 coneEnd = end;

	AddVertsForCylinder3D(verts, cylinderStart, cylinderEnd, radius, color, AABB2::ZERO_TO_ONE, numSlices);
	AddVertsForCone3D(verts, coneStart, coneEnd, coneRadius, color, AABB2::ZERO_TO_ONE, numSlices);
}

void AddVertsForCylinderZ3D(std::vector<Vertex_PCU>& verts, Vec2 const& centerXY, FloatRange const& minMaxZ, float radius, int numSlices /*= 32*/, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);

	Vec3 bottomCenter	= Vec3(centerXY.x, centerXY.y, minMaxZ.m_min);
	Vec3 topCenter		= Vec3(centerXY.x, centerXY.y, minMaxZ.m_max);

	for (int i = 0; i < numSlices; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SLICE;
		float secondDegrees = (i + 1) * DEGREES_PER_SLICE;

		//Vec2 first = Vec2::MakeFromPolarDegrees(firstDegrees);
		//Vec2 second = Vec2::MakeFromPolarDegrees(secondDegrees);

		float firstCos = CosDegrees(firstDegrees);
		float firstSin = SinDegrees(firstDegrees);
		float secondCos = CosDegrees(secondDegrees);
		float secondSin = SinDegrees(secondDegrees);


		Vec3 firstOffset = radius * Vec3(firstCos, firstSin, 0.f);
		Vec3 secondOffset = radius * Vec3(secondCos, secondSin, 0.f);

		Vec3 BL = bottomCenter + firstOffset;
		Vec3 BR = bottomCenter + secondOffset;
		Vec3 TL = topCenter + firstOffset;
		Vec3 TR = topCenter + secondOffset;

		Vec2 uvCenter = UVs.GetCenter();
		Vec2 halfDimensions = UVs.GetDimensions() * 0.5f;

		// Top and Bottom Circle
		verts.push_back(Vertex_PCU(bottomCenter, color, uvCenter));
		verts.push_back(Vertex_PCU(BR, color, uvCenter + Vec2(secondCos, -secondSin) * halfDimensions));
		verts.push_back(Vertex_PCU(BL, color, uvCenter + Vec2(firstCos, -firstSin) * halfDimensions));

		verts.push_back(Vertex_PCU(topCenter, color, uvCenter));
		verts.push_back(Vertex_PCU(TL, color, uvCenter + Vec2(firstCos, firstSin) * halfDimensions));
		verts.push_back(Vertex_PCU(TR, color, uvCenter + Vec2(secondCos, secondSin) * halfDimensions));

		// Side Quad
		float sideIncrement = UVs.GetDimensions().x / static_cast<float>(numSlices);
		float minX = UVs.m_mins.x + sideIncrement * static_cast<float>(i);
		float maxX = minX + sideIncrement;

		AddVertsForQuad3D(verts, BL, BR, TR, TL, color, AABB2(minX, UVs.m_mins.y, maxX, UVs.m_maxs.y));
	}
}

void AddVertsForCylinderZ3D(std::vector<Vertex_PCUTBN>& verts, std::vector<unsigned int>& indexes, Vec2 const& centerXY, FloatRange const& minMaxZ, float radius, int numSlices /*= 32*/, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	unsigned int const startIndex = static_cast<unsigned int>(verts.size());
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);

	Vec3 bottomCenter = Vec3(centerXY.x, centerXY.y, minMaxZ.m_min);
	Vec3 topCenter = Vec3(centerXY.x, centerXY.y, minMaxZ.m_max);

	Mat44 localSpace = Mat44();
	Vec3 iBasis = localSpace.GetIBasis3D();
	Vec3 jBasis = localSpace.GetJBasis3D();
	Vec3 kBasis = localSpace.GetKBasis3D();

	Vec2 uvCenter = UVs.GetCenter();
	Vec2 halfDimensions = UVs.GetDimensions() * 0.5f;

	// Bottom and Top Center (0, 1)
	verts.push_back(Vertex_PCUTBN(bottomCenter, color, uvCenter, iBasis, -jBasis, -kBasis));
	verts.push_back(Vertex_PCUTBN(topCenter, color, uvCenter, iBasis, jBasis, kBasis));

	for (int i = 0; i <= numSlices; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SLICE;

		float firstCos = CosDegrees(firstDegrees);
		float firstSin = SinDegrees(firstDegrees);


		Vec3 firstOffset = iBasis * radius * firstCos + jBasis * radius * firstSin;

		Vec3 BL = bottomCenter + firstOffset;
		Vec3 TL = topCenter + firstOffset;

		// Bottom
		verts.push_back(Vertex_PCUTBN(BL, color, uvCenter + Vec2(firstCos, -firstSin) * halfDimensions, iBasis, -jBasis, -kBasis));

		// Top
		verts.push_back(Vertex_PCUTBN(TL, color, uvCenter + Vec2(firstCos, firstSin) * halfDimensions, iBasis, jBasis, kBasis));

		// Side
		float sideIncrement = UVs.GetDimensions().x / static_cast<float>(numSlices);
		float minX = UVs.m_mins.x + sideIncrement * static_cast<float>(i);

		Vec3 firstNormal = iBasis * firstCos + jBasis * firstSin;
		Vec3 tangent = CrossProduct3D(kBasis, firstNormal);

		verts.push_back(Vertex_PCUTBN(BL, color, Vec2(minX, UVs.m_mins.y), tangent, kBasis, firstNormal));
		verts.push_back(Vertex_PCUTBN(TL, color, Vec2(minX, UVs.m_maxs.y), tangent, kBasis, firstNormal));
	}

	for (int i = 0; i < numSlices; ++i)
	{
		int offset = startIndex + 4 * i + 2;
		indexes.push_back(startIndex);	// Bottom Center
		indexes.push_back(offset + 4);	// BR
		indexes.push_back(offset);		// BL

		indexes.push_back(startIndex + 1);	// Top Center
		indexes.push_back(offset + 1);		// TL
		indexes.push_back(offset + 5);		// TR

		indexes.push_back(offset + 2);	// Quad BL
		indexes.push_back(offset + 6);	// Quad BR
		indexes.push_back(offset + 7);	// Quad TR

		indexes.push_back(offset + 2);	// Quad BL
		indexes.push_back(offset + 7);	// Quad TR
		indexes.push_back(offset + 3);	// Quad TL
	}
}

void AddVertsForOBB3(std::vector<Vertex_PCU>& verts, OBB3 const& bounds, Rgba8 const& color, AABB2 const& UVs /*= AABB2::ZERO_TO_ONE*/)
{
	std::vector<Vertex_PCU> tempVerts;
	AABB3 localOBB = AABB3(bounds.m_halfDimensions, bounds.m_halfDimensions);
	AddVertsForAABB3D(tempVerts, localOBB, color, UVs);
	Mat44 obbMat = Mat44(bounds.m_iBasisNormal, bounds.m_jBasisNormal, bounds.m_kBasisNormal, bounds.m_center);

	for (Vertex_PCU vertex : tempVerts)
	{
		Vec3 worldPos = obbMat.TransformPosition3D(vertex.m_position);
		verts.push_back(Vertex_PCU(worldPos, vertex.m_color, vertex.m_uvTexCoords));
	}
}


void AddVertsForPenumbra3D(std::vector<Vertex_PCU>& verts, Vec3 const& position, Vec3 const& fwdNormal, float radius, float penumbraDot, Rgba8 const& color /*= Rgba8::OPAQUE_WHITE*/, int numSlices /*= 16*/)
{
	float const DEGREES_PER_SLICE = 360.f / static_cast<float>(numSlices);

	float cosHalfAperture = GetClamped(penumbraDot, -1.f, 1.f);
	float coneRadius = radius * sqrtf(GetClampedZeroToOne(1.f - cosHalfAperture * cosHalfAperture));

	Vec3 start = position;
	Vec3 end = start + fwdNormal * radius * cosHalfAperture;

	Mat44 localSpace = Mat44::MakeFromX(end - start);
	Vec3 jBasis = localSpace.GetJBasis3D();
	Vec3 kBasis = localSpace.GetKBasis3D();

	for (int i = 0; i < numSlices; ++i)
	{
		float firstDegrees = i * DEGREES_PER_SLICE;
		float secondDegrees = (i + 1) * DEGREES_PER_SLICE;

		Vec3 firstOffset = jBasis * coneRadius * CosDegrees(firstDegrees) + kBasis * coneRadius * SinDegrees(firstDegrees);
		Vec3 secondOffset = jBasis * coneRadius * CosDegrees(secondDegrees) + kBasis * coneRadius * SinDegrees(secondDegrees);

		Vec3 BL = end + firstOffset;
		Vec3 BR = end + secondOffset;

		verts.push_back(Vertex_PCU(start, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BL, color, Vec2::ZERO));
		verts.push_back(Vertex_PCU(BR, color, Vec2::ZERO));
	}
}

//-----------------------------------------------------------------------------------------------
void AddVertsForGridXY(std::vector<Vertex_PCU>& verts, IntVec2 dimensions)
{
	constexpr float ORIGIN_LINE_WIDTH = 0.1f;
	constexpr float MAJOR_LINE_WIDTH = 0.05f;
	constexpr float MINOR_LINE_WIDTH = 0.025f;

	float xAxisLength = static_cast<float>(dimensions.x);
	float yAxisLength = static_cast<float>(dimensions.y);

	int xMax = (dimensions.x / 2);
	int xMin = -xMax;

	for (int x = xMin; x <= xMax; ++x)
	{
		float lineThickness = MINOR_LINE_WIDTH;
		Rgba8 lineColor = Rgba8(128, 128, 128);

		if (x == 0)
		{
			lineThickness = ORIGIN_LINE_WIDTH;
			lineColor = Rgba8::GREEN;
		}
		else if (x % 5 == 0)
		{
			lineThickness = MAJOR_LINE_WIDTH;
			lineColor = Rgba8::GREEN;
		}

		AABB3 lineBox(Vec3(), Vec3(lineThickness, yAxisLength, lineThickness));
		lineBox.SetCenter(Vec3((float)x, 0.f, 0.f));
		AddVertsForAABB3D(verts, lineBox, lineColor);
	}

	int yMax = (dimensions.y / 2);
	int yMin = -yMax;

	for (int y = yMin; y <= yMax; ++y)
	{
		float lineThickness = MINOR_LINE_WIDTH;
		Rgba8 lineColor = Rgba8(128, 128, 128);

		if (y == 0)
		{
			lineThickness = ORIGIN_LINE_WIDTH;
			lineColor = Rgba8::RED;
		}
		else if (y % 5 == 0)
		{
			lineThickness = MAJOR_LINE_WIDTH;
			lineColor = Rgba8::RED;
		}

		AABB3 lineBox(Vec3(), Vec3(xAxisLength, lineThickness, lineThickness));
		lineBox.SetCenter(Vec3(0.f, (float)y, 0.f));
		AddVertsForAABB3D(verts, lineBox, lineColor);
	}
}

void AddVertsForGridPlane3D(std::vector<Vertex_PCU>& verts, Plane3 const& plane, Vec3 const& midPerpdicularPoint /*= Vec3::ZERO*/, Vec2 const& gridSize /*= Vec2(100.f, 100.f)*/, Vec2 const& cellSize /*= Vec2(1.f, 1.f)*/, float gridThickness /*= 0.025f*/, Rgba8 xAxisColor /*= Rgba8::RED*/, Rgba8 yAxisColor /*= Rgba8::GREEN*/)
{
	constexpr unsigned char MAJOR_AXIS_ALPHA = 200;
	constexpr unsigned char MINOR_AXIS_ALPHA = 100;

	Vec3 gridCenter = plane.GetNearestPoint(midPerpdicularPoint);
	Mat44 gridTransform = Mat44::MakeFromZ(plane.m_normal);
	// kBasis - normal, iBasis - x Axis, jBasis - y Axis
	gridTransform.SetTranslation3D(gridCenter);

	int xMax = static_cast<int>((gridSize.x / 2.f) / cellSize.x);
	int xMin = -xMax;

	int yMax = static_cast<int>((gridSize.y / 2.f) / cellSize.y);
	int yMin = -yMax;

	std::vector<Vertex_PCU> localVerts;
	localVerts.reserve(48 * (xMax + yMax + 1));

	for (int x = xMin; x <= xMax; ++x)
	{
		Rgba8 lineColor;

		if (x == 0)
		{
			lineColor = Rgba8(yAxisColor.r, yAxisColor.g, yAxisColor.b, MAJOR_AXIS_ALPHA);
		}
		else
		{
			lineColor = Rgba8(yAxisColor.r, yAxisColor.g, yAxisColor.b, MINOR_AXIS_ALPHA);
		}

		AABB3 lineBox(Vec3(), Vec3(gridThickness, gridSize.y, gridThickness));
		lineBox.SetCenter(Vec3((float)x, 0.f, 0.f));
		AddVertsForAABB3D(localVerts, lineBox, lineColor);
	}


	for (int y = yMin; y <= xMax; ++y)
	{
		Rgba8 lineColor;

		if (y == 0)
		{
			lineColor = Rgba8(xAxisColor.r, xAxisColor.g, xAxisColor.b, MAJOR_AXIS_ALPHA);
		}
		else
		{
			lineColor = Rgba8(xAxisColor.r, xAxisColor.g, xAxisColor.b, MINOR_AXIS_ALPHA);
		}

		AABB3 lineBox(Vec3(), Vec3(gridSize.x, gridThickness, gridThickness));
		lineBox.SetCenter(Vec3(0.f, (float)y, 0.f));
		AddVertsForAABB3D(localVerts, lineBox, lineColor);
	}

	for (Vertex_PCU const& vertex : localVerts)
	{
		Vec3 worldPos = gridTransform.TransformPosition3D(vertex.m_position);
		verts.push_back(Vertex_PCU(worldPos, vertex.m_color, vertex.m_uvTexCoords));
	}
}

