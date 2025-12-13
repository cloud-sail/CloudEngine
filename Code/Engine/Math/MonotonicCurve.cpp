#include "Engine/Math/MonotonicCurve.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include <algorithm>

//-----------------------------------------------------------------------------------------------
MonotonicCurve::MonotonicCurve(std::vector<Vec2> const& points)
	: m_points(points)
{
	std::sort(m_points.begin(), m_points.end(),
		[](Vec2 const& a, Vec2 const& b) { return a.x < b.x; });

	ValidateMonotonicity();
}

//-----------------------------------------------------------------------------------------------
void MonotonicCurve::AddPoint(float x, float y)
{
	AddPoint(Vec2(x, y));
}

//-----------------------------------------------------------------------------------------------
void MonotonicCurve::AddPoint(Vec2 const& point)
{
	auto it = std::lower_bound(m_points.begin(), m_points.end(), point,
		[](Vec2 const& a, Vec2 const& b) { return a.x < b.x; });
	m_points.insert(it, point);

	ValidateMonotonicity();
}

//-----------------------------------------------------------------------------------------------
void MonotonicCurve::Clear()
{
	m_points.clear();
}

//-----------------------------------------------------------------------------------------------
void MonotonicCurve::ValidateMonotonicity() const
{
	if (m_points.size() < 2)
		return;

	// Determine expected direction from first two points
	bool shouldIncrease = m_points[1].y >= m_points[0].y;

	// Check all consecutive pairs maintain strict monotonicity
	for (size_t i = 0; i < m_points.size() - 1; ++i)
	{
		float y0 = m_points[i].y;
		float y1 = m_points[i + 1].y;

		if (shouldIncrease && y1 <= y0)
		{
			ERROR_AND_DIE("MonotonicCurve: Y values must be strictly increasing (no equal Y values allowed)");
		}
		else if (!shouldIncrease && y1 >= y0)
		{
			ERROR_AND_DIE("MonotonicCurve: Y values must be strictly decreasing (no equal Y values allowed)");
		}
	}
}

//-----------------------------------------------------------------------------------------------
float MonotonicCurve::EvaluateX(float x) const
{
	if (m_points.empty()) return 0.f;
	if (m_points.size() == 1) return m_points[0].y;

	float clampedX = GetClamped(x, m_points.front().x, m_points.back().x);
	if (clampedX <= m_points.front().x) return m_points.front().y;
	if (clampedX >= m_points.back().x) return m_points.back().y;

	for (size_t i = 0; i < m_points.size() - 1; ++i)
	{
		if (clampedX >= m_points[i].x && clampedX <= m_points[i + 1].x)
		{
			float t = GetFractionWithinRange(clampedX, m_points[i].x, m_points[i + 1].x);
			return Interpolate(m_points[i].y, m_points[i + 1].y, t);
		}
	}

	return m_points.back().y;
}

//-----------------------------------------------------------------------------------------------
float MonotonicCurve::EvaluateY(float y) const
{
	if (m_points.empty()) return 0.f;
	if (m_points.size() == 1) return m_points[0].x;

	bool isIncreasing = m_points.back().y >= m_points.front().y;
	float minY = isIncreasing ? m_points.front().y : m_points.back().y;
	float maxY = isIncreasing ? m_points.back().y : m_points.front().y;

	float clampedY = GetClamped(y, minY, maxY);

	if (isIncreasing)
	{
		if (clampedY <= m_points.front().y) return m_points.front().x;
		if (clampedY >= m_points.back().y) return m_points.back().x;
	}
	else
	{
		if (clampedY >= m_points.front().y) return m_points.front().x;
		if (clampedY <= m_points.back().y) return m_points.back().x;
	}

	for (size_t i = 0; i < m_points.size() - 1; ++i)
	{
		float y0 = m_points[i].y;
		float y1 = m_points[i + 1].y;

		bool inSegment = isIncreasing ? (clampedY >= y0 && clampedY <= y1)
			: (clampedY <= y0 && clampedY >= y1);

		if (inSegment)
		{
			float t = GetFractionWithinRange(clampedY, y0, y1);
			return Interpolate(m_points[i].x, m_points[i + 1].x, t);
		}
	}

	return m_points.back().x;
}

//-----------------------------------------------------------------------------------------------
float MonotonicCurve::GetMinX() const
{
	return m_points.empty() ? 0.f : m_points.front().x;
}

//-----------------------------------------------------------------------------------------------
float MonotonicCurve::GetMaxX() const
{
	return m_points.empty() ? 0.f : m_points.back().x;
}

//-----------------------------------------------------------------------------------------------
float MonotonicCurve::GetMinY() const
{
	if (m_points.empty()) return 0.f;

	bool isIncreasing = m_points.back().y >= m_points.front().y;
	return isIncreasing ? m_points.front().y : m_points.back().y;
}

//-----------------------------------------------------------------------------------------------
float MonotonicCurve::GetMaxY() const
{
	if (m_points.empty()) return 0.f;

	bool isIncreasing = m_points.back().y >= m_points.front().y;
	return isIncreasing ? m_points.back().y : m_points.front().y;
}

//-----------------------------------------------------------------------------------------------
Vec2 MonotonicCurve::GetPoint(int index) const
{
	if (index < 0 || index >= static_cast<int>(m_points.size()))
		return Vec2::ZERO;
	return m_points[index];
}