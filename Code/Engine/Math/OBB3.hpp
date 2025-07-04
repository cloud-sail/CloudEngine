#pragma once
#include "Engine/Math/Vec3.hpp"

struct Mat44;

struct OBB3
{
public:
	Vec3 m_center;
	Vec3 m_iBasisNormal;
	Vec3 m_jBasisNormal;
	Vec3 m_kBasisNormal;
	Vec3 m_halfDimensions;

public:
	OBB3();
	~OBB3() = default;
	explicit OBB3(Vec3 const& center, Vec3 const& iBasisNormal, Vec3 const& jBasisNormal, Vec3 const& kBasisNormal, Vec3 const& halfDimensions);

	Mat44 GetLocalToWorldTransform() const;

	void GetCornerPoints(Vec3* out_eightCornerPositions) const;
	Vec3 const GetLocalPosForWorldPos(Vec3 const& worldPos) const;
	Vec3 const GetWorldPosForLocalPos(Vec3 const& localPos) const;

	Vec3 const GetNearestPoint(Vec3 const& referencePosition) const;
};

