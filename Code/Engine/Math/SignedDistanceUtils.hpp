#pragma once
#include <cmath>
#include <algorithm>

struct Vec2;
struct Vec3;
struct OBB2;

namespace SdfPrimitives
{
	// Local space, centered at origin

	// https://iquilezles.org/articles/distfunctions2d/
	// r - radius
	float Disc2D(Vec2 const& p, float radius);
	// h - half dimensions
	float Box2D(Vec2 const& p, Vec2 const& halfDimensions);
	// a - start of middle axis, b - end of middle axis, th - thickness
	float OrientedBox2D(Vec2 const& p, Vec2 const& a, Vec2 const& b, float th);
	float OrientedBox2D(Vec2 const& p, OBB2 const& box);

	float Segment2D(Vec2 const& p, Vec2 const& start, Vec2 const& end);
	float Capsule2D(Vec2 const& p, Vec2 const& boneStart, Vec2 const& boneEnd, float radius);

	// r - radius
	float Sphere3D(Vec3 const& p, float radius);
	// h - half dimensions
	float Box3D(Vec3 const& p, Vec3 const& halfDimensions);

	float Segment3D(Vec3 const& p, Vec3 const& start, Vec3 const& end);
	
};

namespace SdfOps
{
	inline float Union(float a, float b)
	{
		return std::min(a, b);
	}

	inline float Subtraction(float a, float b)
	{
		return std::max(-a, b);
	}

	inline float Intersection(float a, float b)
	{
		return std::max(a, b);
	}

	inline float Xor(float a, float b)
	{
		return std::max(std::min(a, b), -std::max(a, b));
	}

	// https://iquilezles.org/articles/smin/
	// max(x, y) = -min(-x, -y)
	inline float SmoothUnionQuadratic(float a, float b, float k)
	{
		k *= 4.0f;
		float h = std::max(k - fabsf(a - b), 0.f) / k;
		return std::min(a, b) - h * h * k * (1.0f / 4.0f);
	}

	inline float SmoothSubtractionQuadratic(float a, float b, float k)
	{
		return -SmoothUnionQuadratic(a, -b, k);
	}

	inline float SmoothIntersectionQuadratic(float a, float b, float k)
	{
		return -SmoothUnionQuadratic(-a, -b, k);
	}

	inline float SmoothUnionCubic(float a, float b, float k)
	{
		k *= 6.0f;
		float h = std::max(k - fabsf(a - b), 0.f) / k;
		return std::min(a, b) - h * h * h * k * (1.0f / 6.0f);
	}

	inline float SmoothSubtractionCubic(float a, float b, float k)
	{
		return -SmoothUnionCubic(a, -b, k);
	}

	inline float SmoothIntersectionCubic(float a, float b, float k)
	{
		return -SmoothUnionCubic(-a, -b, k);
	}
}

