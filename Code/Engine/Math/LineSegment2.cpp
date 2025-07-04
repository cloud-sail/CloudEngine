#include "Engine/Math/LineSegment2.hpp"

LineSegment2::LineSegment2(Vec2 const& start, Vec2 const& end)
	: m_start(start)
	, m_end(end)
{
}

void LineSegment2::Translate(Vec2 translation)
{
	m_start += translation;
	m_end += translation;
}

void LineSegment2::SetCenter(Vec2 newCenter)
{
	Vec2 translation = newCenter - 0.5f * (m_start + m_end);
	Translate(translation);
}

void LineSegment2::RotateAboutCenter(float rotationDeltaDegrees)
{
	Vec2 center = 0.5f * (m_start + m_end);
	Vec2 halfSegment = 0.5f * (m_end - m_start);
	Vec2 newHalfSegment = halfSegment.GetRotatedDegrees(rotationDeltaDegrees);
	m_end = center + newHalfSegment;
	m_start = center - newHalfSegment;
}


