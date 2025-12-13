#include "Engine/Math/IntBox3.hpp"
#include "Engine/Math/AABB3.hpp"
#include <cmath>


IntBox3::IntBox3(IntVec3 const& mins, IntVec3 const& dimensions)
	: m_mins(mins)
	, m_dimensions(dimensions)
{
}


IntBox3 IntBox3::MakeFromAABB3(AABB3 const& aabb)
{
	IntVec3 mins(
		static_cast<int>(floorf(aabb.m_mins.x)),
		static_cast<int>(floorf(aabb.m_mins.y)),
		static_cast<int>(floorf(aabb.m_mins.z))
	);

	IntVec3 maxs(
		static_cast<int>(ceilf(aabb.m_maxs.x)),
		static_cast<int>(ceilf(aabb.m_maxs.y)),
		static_cast<int>(ceilf(aabb.m_maxs.z))
	);

	return IntBox3(mins, maxs - mins);
}

bool IntBox3::IsPointInside(IntVec3 point) const
{
	IntVec3 maxs = m_mins + m_dimensions;

	return point.x >= m_mins.x && point.x < maxs.x &&
		point.y >= m_mins.y && point.y < maxs.y &&
		point.z >= m_mins.z && point.z < maxs.z;
}

bool IntBox3::IsOverlap(const IntBox3& other) const
{
	IntVec3 maxs = m_mins + m_dimensions;
	IntVec3 otherMaxs = other.m_mins + other.m_dimensions;

	if (maxs.x <= other.m_mins.x || m_mins.x >= otherMaxs.x) return false;
	if (maxs.y <= other.m_mins.y || m_mins.y >= otherMaxs.y) return false;
	if (maxs.z <= other.m_mins.z || m_mins.z >= otherMaxs.z) return false;

	return true;
}

void IntBox3::ClampWithIn(IntBox3 const& containingBox)
{
	IntVec3 newMins = m_mins;
	IntVec3 newMaxs = m_mins + m_dimensions;

	IntVec3 containingMins = containingBox.m_mins;
	IntVec3 containingMaxs = containingMins + containingBox.m_dimensions;

	if (containingMins.x > newMins.x)
	{
		newMins.x = containingMins.x;
	}
	if (containingMins.y > newMins.y)
	{
		newMins.y = containingMins.y;
	}
	if (containingMins.z > newMins.z)
	{
		newMins.z = containingMins.z;
	}

	if (containingMaxs.x < newMaxs.x)
	{
		newMaxs.x = containingMaxs.x;
	}
	if (containingMaxs.y < newMaxs.y)
	{
		newMaxs.y = containingMaxs.y;
	}
	if (containingMaxs.z < newMaxs.z)
	{
		newMaxs.z = containingMaxs.z;
	}

	m_mins = newMins;
	m_dimensions = newMaxs - newMins;
}

IntBox3 IntBox3::GetUnionWith(IntBox3 const& other) const
{
	IntVec3 thisMins = m_mins;
	IntVec3 thisMaxs = m_mins + m_dimensions;

	IntVec3 otherMins = other.m_mins;
	IntVec3 otherMaxs = otherMins + other.m_dimensions;

	IntVec3 newMins;
	newMins.x = (thisMins.x < otherMins.x) ? thisMins.x : otherMins.x;
	newMins.y = (thisMins.y < otherMins.y) ? thisMins.y : otherMins.y;
	newMins.z = (thisMins.z < otherMins.z) ? thisMins.z : otherMins.z;

	IntVec3 newMaxs;
	newMaxs.x = (thisMaxs.x > otherMaxs.x) ? thisMaxs.x : otherMaxs.x;
	newMaxs.y = (thisMaxs.y > otherMaxs.y) ? thisMaxs.y : otherMaxs.y;
	newMaxs.z = (thisMaxs.z > otherMaxs.z) ? thisMaxs.z : otherMaxs.z;

	return IntBox3(newMins, newMaxs - newMins);
}
