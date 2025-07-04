#pragma once
#include "Engine/Math/Vec2.hpp"

struct AABB2
{
public:
	Vec2 m_mins;
	Vec2 m_maxs;

	static const AABB2 ZERO_TO_ONE;

public:
	// Construction/Destruction
	~AABB2() = default;
	AABB2() = default;
	AABB2(AABB2 const& copyFrom);
	explicit AABB2(float minX, float minY, float maxX, float maxY);
	explicit AABB2(Vec2 const& mins, Vec2 const& maxs);

	// Accessors (const methods)
	bool		IsPointInside(Vec2 const& point) const;
	Vec2 const	GetCenter() const;
	Vec2 const	GetDimensions() const;
	Vec2 const	GetNearestPoint(Vec2 const& referencePosition) const;
	Vec2 const	GetPointAtUV(Vec2 const& uv) const;				// uv=(0,0) is at mins; uv=(1,1) is at maxs
	Vec2 const	GetUVForPoint(Vec2 const& point) const;			// uv=(.5,.5) at center; u or v outside [0,1] extrapolated

	// Mutators (non-const methods)
	void Translate(Vec2 const& translationToApply);
	void SetCenter(Vec2 const& newCenter);
	void SetDimensions(Vec2 const& newDimensions);
	void StretchToIncludePoint(Vec2 const& point); // does minimal stretching required (none if already on point)

	void ClampWithin(AABB2 const& containingBox);
	void AddPadding(float xToAddOnBothSides, float yToAddToTopAndBottom);
	void ReduceToAspect(float newAspect);
	void EnlargeToAspect(float newAspect);

	void ChopOffTop(float percentOfOriginalToChop, float extraHeightOfOriginalToChop = 0.f);
	void ChopOffBottom(float percentOfOriginalToChop, float extraHeightOfOriginalToChop = 0.f);
	void ChopOffLeft(float percentOfOriginalToChop, float extraHeightOfOriginalToChop = 0.f);
	void ChopOffRight(float percentOfOriginalToChop, float extraHeightOfOriginalToChop = 0.f);


	// operators
	bool operator==(AABB2 const& rightHandSide) const;
};


