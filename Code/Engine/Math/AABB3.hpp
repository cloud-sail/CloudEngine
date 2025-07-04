#pragma once
#include "Engine/Math/Vec3.hpp"



struct AABB3
{
public:
	Vec3 m_mins;
	Vec3 m_maxs;

public:
	~AABB3();
	AABB3();
	AABB3(AABB3 const& copyFrom); // copy constructor (from another AABB3)
	explicit AABB3(float minX, float minY, float minZ, float maxX, float maxY, float maxZ); // explicit construct
	explicit AABB3(Vec3 const& mins, Vec3 const& maxs);


	// Accessors (const methods)
	bool IsPointInside(Vec3 const& point) const;
	Vec3 const	GetNearestPoint(Vec3 const& referencePosition) const;
	Vec3 const	GetCenter() const;
	void GetCornerPoints(Vec3* out_eightCornerPositions) const;

	// Mutators (non-const methods)
	void Translate(Vec3 const& translationToApply);
	void SetCenter(Vec3 const& newCenter);
};


