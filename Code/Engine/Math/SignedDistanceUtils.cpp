#include "Engine/Math/SignedDistanceUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

float SdfPrimitives::Disc2D(Vec2 const& p, float r)
{
	return p.GetLength() - r;
}

float SdfPrimitives::Box2D(Vec2 const& p, Vec2 const& h)
{
	Vec2 q = Vec2(fabsf(p.x), fabsf(p.y)) - h;
	float outside = Vec2(std::max(q.x, 0.f), std::max(q.y, 0.f)).GetLength();
	float inside = std::min(std::max(q.x, q.y), 0.f);
	return outside + inside;
}

float SdfPrimitives::OrientedBox2D(Vec2 const& p, Vec2 const& a, Vec2 const& b, float th)
{
	float l = (b - a).GetLength();
	Vec2  d = (b - a) / l;

	Vec2 q = p - (a + b) * 0.5f;

	q = Vec2(DotProduct2D(q, d), CrossProduct2D(d, q));

	q = Vec2(fabsf(q.x), fabsf(q.y)) - Vec2(l, th) * 0.5f;

	float outside = Vec2(std::max(q.x, 0.f), std::max(q.y, 0.f)).GetLength();
	float inside = std::min(std::max(q.x, q.y), 0.f);
	return outside + inside;
}

float SdfPrimitives::OrientedBox2D(Vec2 const& p, OBB2 const& box)
{
	Vec2 localQ = box.GetLocalPosForWorldPos(p);

	localQ = Vec2(fabsf(localQ.x), fabsf(localQ.y)) - box.m_halfDimensions;

	float outside = Vec2(std::max(localQ.x, 0.f), std::max(localQ.y, 0.f)).GetLength();
	float inside = std::min(std::max(localQ.x, localQ.y), 0.f);
	return outside + inside;
}

float SdfPrimitives::Segment2D(Vec2 const& p, Vec2 const& a, Vec2 const& b)
{
	Vec2 aToP = p - a;
	Vec2 aToB = b - a;
	float h = GetClampedZeroToOne(DotProduct2D(aToP, aToB) / DotProduct2D(aToB, aToB));

	return (aToP - aToB * h).GetLength();
}

float SdfPrimitives::Capsule2D(Vec2 const& p, Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
{
	return SdfPrimitives::Segment2D(p, boneStart, boneEnd) - radius;
}

float SdfPrimitives::Sphere3D(Vec3 const& p, float r)
{
	return p.GetLength() - r;
}

float SdfPrimitives::Box3D(Vec3 const& p, Vec3 const& h)
{
	Vec3 q = Vec3(fabsf(p.x), fabsf(p.y), fabsf(p.z)) - h;
	float outside = Vec3(std::max(q.x, 0.f), std::max(q.y, 0.f), std::max(q.z, 0.f)).GetLength();
	float inside = std::min(std::max(q.x, std::max(q.y, q.z)), 0.f);

	return outside + inside;
}

float SdfPrimitives::Segment3D(Vec3 const& p, Vec3 const& a, Vec3 const& b)
{
	Vec3 aToP = p - a;
	Vec3 aToB = b - a;
	float h = GetClampedZeroToOne(DotProduct3D(aToP, aToB) / DotProduct3D(aToB, aToB));

	return (aToP - aToB * h).GetLength();
}
