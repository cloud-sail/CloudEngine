#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

//-----------------------------------------------------------------------------------------------
const AABB2 AABB2::ZERO_TO_ONE(0.f, 0.f, 1.f, 1.f);

//-----------------------------------------------------------------------------------------------

AABB2::AABB2(AABB2 const& copyFrom)
	: m_mins(copyFrom.m_mins)
	, m_maxs(copyFrom.m_maxs)
{

}

AABB2::AABB2(float minX, float minY, float maxX, float maxY)
	: m_mins(Vec2(minX, minY))
	, m_maxs(Vec2(maxX, maxY))
{
}

AABB2::AABB2(Vec2 const& mins, Vec2 const& maxs)
	: m_mins(mins)
	, m_maxs(maxs)
{
}

bool AABB2::IsPointInside(Vec2 const& point) const
{
	return (point.x > m_mins.x && point.x < m_maxs.x &&
		point.y > m_mins.y && point.y < m_maxs.y);
}

Vec2 const AABB2::GetCenter() const
{
	return (m_maxs + m_mins) * 0.5f;
}

Vec2 const AABB2::GetDimensions() const
{
	return (m_maxs - m_mins);
}

Vec2 const AABB2::GetNearestPoint(Vec2 const& referencePosition) const
{
	float nearestX = GetClamped(referencePosition.x, m_mins.x, m_maxs.x);
	float nearestY = GetClamped(referencePosition.y, m_mins.y, m_maxs.y);
	return Vec2(nearestX, nearestY);
}

Vec2 const AABB2::GetPointAtUV(Vec2 const& uv) const
{
	return Vec2(Interpolate(m_mins.x, m_maxs.x, uv.x),
				Interpolate(m_mins.y, m_maxs.y, uv.y));
}

Vec2 const AABB2::GetUVForPoint(Vec2 const& point) const
{
	return Vec2(GetFractionWithinRange(point.x, m_mins.x, m_maxs.x),
				GetFractionWithinRange(point.y, m_mins.y, m_maxs.y));
}

void AABB2::Translate(Vec2 const& translationToApply)
{
	m_mins += translationToApply;
	m_maxs += translationToApply;
}

void AABB2::SetCenter(Vec2 const& newCenter)
{
	Vec2 const translationToApply = newCenter - GetCenter();
	Translate(translationToApply);
}

void AABB2::SetDimensions(Vec2 const& newDimensions)
{
	Vec2 const center = GetCenter();
	Vec2 const newRadius = newDimensions * 0.5f;
	m_mins = center - newRadius;
	m_maxs = center + newRadius;
}

void AABB2::StretchToIncludePoint(Vec2 const& point)
{
	if (point.x > m_maxs.x) m_maxs.x = point.x;
	if (point.x < m_mins.x) m_mins.x = point.x;
	if (point.y > m_maxs.y) m_maxs.y = point.y;
	if (point.y < m_mins.y) m_mins.y = point.y;
}

void AABB2::ClampWithin(AABB2 const& containingBox)
{
	if (containingBox.m_mins.x > m_mins.x)
	{
		m_mins.x = containingBox.m_mins.x;
	}
	if (containingBox.m_mins.y > m_mins.y)
	{
		m_mins.y = containingBox.m_mins.y;
	}
	if (containingBox.m_maxs.x < m_maxs.x)
	{
		m_maxs.x = containingBox.m_maxs.x;
	}
	if (containingBox.m_maxs.y < m_maxs.y)
	{
		m_maxs.y = containingBox.m_maxs.y;
	}
}

void AABB2::AddPadding(float xToAddOnBothSides, float yToAddToTopAndBottom)
{
	m_mins.x -= xToAddOnBothSides;
	m_maxs.x += xToAddOnBothSides;
	m_mins.y -= yToAddToTopAndBottom;
	m_maxs.y += yToAddToTopAndBottom;
}

void AABB2::ReduceToAspect(float newAspect)
{
	float currentWidth = m_maxs.x - m_mins.x;
	float currentHeight = m_maxs.y - m_mins.y;
	float currentAspect = currentWidth / currentHeight;

	if (currentAspect > newAspect)
	{
		float newWidth = newAspect * currentHeight;
		float adjustXOnBothSides = (currentWidth - newWidth) * 0.5f;
		m_mins.x += adjustXOnBothSides;
		m_maxs.x -= adjustXOnBothSides;
	}
	else if (currentAspect < newAspect)
	{
		float newHeight = currentWidth / newAspect;
		float adjustYOnBothSides = (currentHeight - newHeight) * 0.5f;
		m_mins.y += adjustYOnBothSides;
		m_maxs.y -= adjustYOnBothSides;
	}
}

void AABB2::EnlargeToAspect(float newAspect)
{
	float currentWidth = m_maxs.x - m_mins.x;
	float currentHeight = m_maxs.y - m_mins.y;
	float currentAspect = currentWidth / currentHeight;

	if (currentAspect < newAspect)
	{
		float newWidth = newAspect * currentHeight;
		float adjustXOnBothSides = (currentWidth - newWidth) * 0.5f;
		m_mins.x += adjustXOnBothSides;
		m_maxs.x -= adjustXOnBothSides;
	}
	else if (currentAspect > newAspect)
	{
		float newHeight = currentWidth / newAspect;
		float adjustYOnBothSides = (currentHeight - newHeight) * 0.5f;
		m_mins.y += adjustYOnBothSides;
		m_maxs.y -= adjustYOnBothSides;
	}
}

void AABB2::ChopOffTop(float percentOfOriginalToChop, float extraHeightOfOriginalToChop)
{
	float currentHeight = m_maxs.y - m_mins.y;
	float chopHeight = percentOfOriginalToChop * currentHeight + extraHeightOfOriginalToChop;
	if (chopHeight > currentHeight)
	{
		chopHeight = currentHeight; // prevent over chopping
	}

	m_maxs.y -= chopHeight;
}

void AABB2::ChopOffBottom(float percentOfOriginalToChop, float extraHeightOfOriginalToChop)
{
	float currentHeight = m_maxs.y - m_mins.y;
	float chopHeight = percentOfOriginalToChop * currentHeight + extraHeightOfOriginalToChop;
	if (chopHeight > currentHeight)
	{
		chopHeight = currentHeight; // prevent over chopping
	}

	m_mins.y += chopHeight;
}

void AABB2::ChopOffLeft(float percentOfOriginalToChop, float extraHeightOfOriginalToChop)
{
	float currentWidth = m_maxs.x - m_mins.x;
	float chopWidth = percentOfOriginalToChop * currentWidth + extraHeightOfOriginalToChop;
	if (chopWidth > currentWidth)
	{
		chopWidth = currentWidth;
	}

	m_mins.x += chopWidth;
}

void AABB2::ChopOffRight(float percentOfOriginalToChop, float extraHeightOfOriginalToChop)
{
	float currentWidth = m_maxs.x - m_mins.x;
	float chopWidth = percentOfOriginalToChop * currentWidth + extraHeightOfOriginalToChop;
	if (chopWidth > currentWidth)
	{
		chopWidth = currentWidth;
	}

	m_maxs.x -= chopWidth;
}

bool AABB2::operator==(AABB2 const& rightHandSide) const
{
	return m_mins == rightHandSide.m_mins &&
		m_maxs == rightHandSide.m_maxs;
}
