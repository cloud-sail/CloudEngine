#include "Engine/Math/Plane3.hpp"
#include "Engine/Math/MathUtils.hpp"

Plane3::Plane3(Vec3 const& normal, float distance)
	: m_normal(normal)
	, m_distance(distance)
{
}

Plane3::Plane3(Vec3 const& normal, Vec3 const& pointOnPlane)
	: m_normal(normal)
{
	m_distance = DotProduct3D(normal, pointOnPlane);
}

Plane3::Plane3(Vec3 const& ccw0, Vec3 const& ccw1, Vec3 const& ccw2)
{
	m_normal = CrossProduct3D(ccw1 - ccw0, ccw2 - ccw0).GetNormalized();
	m_distance = DotProduct3D(m_normal, ccw0);
}

Plane3 const Plane3::GetFlipped() const
{
	return Plane3(-m_normal, m_distance);
}

Vec3 const Plane3::GetNearestPoint(Vec3 const& referencePos) const
{
	return referencePos - GetSignedDistanceToPoint(referencePos) * m_normal;
}

float Plane3::GetSignedDistanceToPoint(Vec3 const& point) const
{
	return DotProduct3D(m_normal, point) - m_distance;
}

bool Plane3::IsPointInFrontOf(Vec3 const& point) const
{
	return DotProduct3D(m_normal, point) > m_distance;
}

bool Plane3::IsPointBehind(Vec3 const& point) const
{
	return DotProduct3D(m_normal, point) < m_distance;
}

void Plane3::Translate(Vec3 const& translation)
{
	m_distance += DotProduct3D(m_normal, translation);
}

void Plane3::MoveToPoint(Vec3 const& newPoint)
{
	m_distance = DotProduct3D(m_normal, newPoint);
}
