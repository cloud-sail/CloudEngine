#pragma once
#include "Engine/Math/Vec2.hpp"

struct OBB2
{
public:
	Vec2 m_center;
	Vec2 m_iBasisNormal;
	Vec2 m_halfDimensions;

public:
	OBB2(); // maybe not all zero
	~OBB2() = default;
	explicit OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions);

	void GetCornerPoints(Vec2* out_fourCornerWorldPositions) const; // for drawing!
	Vec2 const GetLocalPosForWorldPos(Vec2 const& worldPos) const;
	Vec2 const GetWorldPosForLocalPos(Vec2 const& localPos) const;
	void RotateAboutCenter(float rotationDeltaDegrees);
};

