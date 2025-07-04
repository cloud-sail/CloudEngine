#include "Engine/Math/OBB3.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB3::OBB3()
	: m_center(Vec3::ZERO)
	, m_iBasisNormal(Vec3::FORWARD)
	, m_jBasisNormal(Vec3::LEFT)
	, m_kBasisNormal(Vec3::UP)
	, m_halfDimensions(Vec3(1.f, 1.f, 1.f))
{

}

OBB3::OBB3(Vec3 const& center, Vec3 const& iBasisNormal, Vec3 const& jBasisNormal, Vec3 const& kBasisNormal, Vec3 const& halfDimensions)
	: m_center(center)
	, m_iBasisNormal(iBasisNormal)
	, m_jBasisNormal(jBasisNormal)
	, m_kBasisNormal(kBasisNormal)
	, m_halfDimensions(halfDimensions)
{

}

Mat44 OBB3::GetLocalToWorldTransform() const
{
	return Mat44(m_iBasisNormal, m_jBasisNormal, m_kBasisNormal, m_center);
}

void OBB3::GetCornerPoints(Vec3* out_eightCornerPositions) const
{
	Vec3 i = m_halfDimensions.x * m_iBasisNormal;
	Vec3 j = m_halfDimensions.y * m_jBasisNormal;
	Vec3 k = m_halfDimensions.z * m_kBasisNormal;

	out_eightCornerPositions[0] = m_center - i - j - k;
	out_eightCornerPositions[1] = m_center - i - j + k;
	out_eightCornerPositions[2] = m_center - i + j - k;
	out_eightCornerPositions[3] = m_center - i + j + k;
	out_eightCornerPositions[4] = m_center + i - j - k;
	out_eightCornerPositions[5] = m_center + i - j + k;
	out_eightCornerPositions[6] = m_center + i + j - k;
	out_eightCornerPositions[7] = m_center + i + j + k;
}

Vec3 const OBB3::GetLocalPosForWorldPos(Vec3 const& worldPos) const
{
	Vec3 displacement = worldPos - m_center;
	return Vec3(DotProduct3D(displacement, m_iBasisNormal),
				DotProduct3D(displacement, m_jBasisNormal),
				DotProduct3D(displacement, m_kBasisNormal));
}

Vec3 const OBB3::GetWorldPosForLocalPos(Vec3 const& localPos) const
{
	return localPos.x * m_iBasisNormal + localPos.y * m_jBasisNormal + localPos.z * m_kBasisNormal + m_center;
}

Vec3 const OBB3::GetNearestPoint(Vec3 const& referencePos) const
{
	Vec3 localPos = GetLocalPosForWorldPos(referencePos);
	AABB3 localOBB = AABB3(-m_halfDimensions, m_halfDimensions);
	Vec3 localNearestPoint = localOBB.GetNearestPoint(localPos);
	return GetWorldPosForLocalPos(localNearestPoint);
}
