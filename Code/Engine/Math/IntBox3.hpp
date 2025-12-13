#pragma once
#include "Engine/Math/IntVec3.hpp"

struct AABB3;

struct IntBox3 
{
	IntVec3 m_mins;
	IntVec3 m_dimensions; // [mins, mins + dims)

	IntBox3() = default;
	IntBox3(IntVec3 const& mins, IntVec3 const& dimensions);

	static IntBox3 MakeFromAABB3(AABB3 const& aabb);



	bool IsPointInside(IntVec3 point) const;

	bool IsOverlap(const IntBox3& other) const;


	void ClampWithIn(IntBox3 const& containingBox);

	IntBox3 GetUnionWith(IntBox3 const& other) const;
};
