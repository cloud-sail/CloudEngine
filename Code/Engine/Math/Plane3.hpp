#pragma once
#include "Engine/Math/Vec3.hpp"

struct Plane3
{
public:
	// N * OP = D
	Vec3 m_normal = Vec3::UP;
	float m_distance = 0.f;  // signed distance to the plane from the origin, measured in the direction of the normal.

public:
	Plane3() = default;
	~Plane3() = default;
	explicit Plane3(Vec3 const& normal, float distance);
	explicit Plane3(Vec3 const& normal, Vec3 const& pointOnPlane);
	explicit Plane3(Vec3 const& ccw0, Vec3 const& ccw1, Vec3 const& ccw2);

	Plane3 const GetFlipped() const;
	
	Vec3 const GetNearestPoint(Vec3 const& referencePos) const;
	float GetSignedDistanceToPoint(Vec3 const& point) const; //Altitude
	bool IsPointInFrontOf(Vec3 const& point) const;
	bool IsPointBehind(Vec3 const& point) const;

	void Translate(Vec3 const& translation);
	void MoveToPoint(Vec3 const& newPoint);
};

