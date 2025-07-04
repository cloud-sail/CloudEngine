#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"

AABB3::~AABB3()
{

}

AABB3::AABB3()
{

}

AABB3::AABB3(AABB3 const& copyFrom)
	: m_mins(copyFrom.m_mins)
	, m_maxs(copyFrom.m_maxs)
{

}

AABB3::AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
	: m_mins(Vec3(minX, minY, minZ))
	, m_maxs(Vec3(maxX, maxY, maxZ))
{

}

AABB3::AABB3(Vec3 const& mins, Vec3 const& maxs)
	: m_mins(mins)
	, m_maxs(maxs)
{

}

bool AABB3::IsPointInside(Vec3 const& point) const
{
	return (point.x > m_mins.x && point.x < m_maxs.x &&
			point.y > m_mins.y && point.y < m_maxs.y &&
			point.z > m_mins.z && point.z < m_maxs.z);
}

Vec3 const AABB3::GetNearestPoint(Vec3 const& referencePosition) const
{
	float nearestX = GetClamped(referencePosition.x, m_mins.x, m_maxs.x);
	float nearestY = GetClamped(referencePosition.y, m_mins.y, m_maxs.y);
	float nearestZ = GetClamped(referencePosition.z, m_mins.z, m_maxs.z);
	return Vec3(nearestX, nearestY, nearestZ);
}

Vec3 const AABB3::GetCenter() const
{
	return (m_maxs + m_mins) * 0.5f;
}

void AABB3::GetCornerPoints(Vec3* out_eightCornerPositions) const
{
	out_eightCornerPositions[0] = Vec3(m_mins.x, m_mins.y, m_mins.z);
	out_eightCornerPositions[1] = Vec3(m_mins.x, m_mins.y, m_maxs.z);
	out_eightCornerPositions[2] = Vec3(m_mins.x, m_maxs.y, m_mins.z);
	out_eightCornerPositions[3] = Vec3(m_mins.x, m_maxs.y, m_maxs.z);
	out_eightCornerPositions[4] = Vec3(m_maxs.x, m_mins.y, m_mins.z);
	out_eightCornerPositions[5] = Vec3(m_maxs.x, m_mins.y, m_maxs.z);
	out_eightCornerPositions[6] = Vec3(m_maxs.x, m_maxs.y, m_mins.z);
	out_eightCornerPositions[7] = Vec3(m_maxs.x, m_maxs.y, m_maxs.z);
}

void AABB3::Translate(Vec3 const& translationToApply)
{
	m_mins += translationToApply;
	m_maxs += translationToApply;
}

void AABB3::SetCenter(Vec3 const& newCenter)
{
	Vec3 const translationToApply = newCenter - GetCenter();
	Translate(translationToApply);
}

