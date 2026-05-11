#include "Engine/Math/Plane2.hpp"
#include "Engine/Math/MathUtils.hpp"

Plane2::Plane2(Vec2 const& normal, float distance)
	: m_normal(normal)
	, m_distance(distance)
{

}

Plane2::Plane2(Vec2 const& normal, Vec2 const& pointOnPlane)
	: m_normal(normal)
{
	m_distance = DotProduct2D(normal, pointOnPlane);
}

float Plane2::GetSignedDistanceToPoint(Vec2 const& point) const
{
	return DotProduct2D(m_normal, point) - m_distance;
}

bool Plane2::IsPointInFrontOf(Vec2 const& point) const
{
	return DotProduct2D(m_normal, point) > m_distance;
}

bool Plane2::IsPointBehind(Vec2 const& point) const
{
	return DotProduct2D(m_normal, point) < m_distance;
}
