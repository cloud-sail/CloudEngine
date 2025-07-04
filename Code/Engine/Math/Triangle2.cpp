#include "Engine/Math/Triangle2.hpp"

Triangle2::Triangle2(Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2)
	: m_pointsCounterClockwise{ccw0, ccw1, ccw2}
{
}

void Triangle2::Translate(Vec2 translation)
{
	m_pointsCounterClockwise[0] += translation;
	m_pointsCounterClockwise[1] += translation;
	m_pointsCounterClockwise[2] += translation;
}
