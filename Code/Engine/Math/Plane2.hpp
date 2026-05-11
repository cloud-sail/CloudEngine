#pragma once
#include "Engine/Math/Vec2.hpp"


struct Plane2
{
public:
	// N * OP = D
	Vec2 m_normal = Vec2(0.f, 1.f);
	float m_distance = 0.f; // signed distance to the plane from the origin, measured in the direction of the normal.

public:
	~Plane2() = default;
	Plane2() = default;
	explicit Plane2(Vec2 const& normal, float distance);
	explicit Plane2(Vec2 const& normal, Vec2 const& pointOnPlane);

	float GetSignedDistanceToPoint(Vec2 const& point) const; //Altitude
	bool IsPointInFrontOf(Vec2 const& point) const;
	bool IsPointBehind(Vec2 const& point) const;
};

