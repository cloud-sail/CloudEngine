#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"

OBB2::OBB2()
	: m_center(Vec2::ZERO)
	, m_iBasisNormal(Vec2(1.f, 0.f))
	, m_halfDimensions(Vec2(1.f, 1.f))
{
}

OBB2::OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions)
	: m_center(center)
	, m_iBasisNormal(iBasisNormal)
	, m_halfDimensions(halfDimensions)
{
}

void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
	Vec2 i = m_halfDimensions.x * m_iBasisNormal;
	Vec2 j = m_halfDimensions.y * m_iBasisNormal.GetRotated90Degrees();
	out_fourCornerWorldPositions[0] = m_center - i - j;
	out_fourCornerWorldPositions[1] = m_center + i - j;
	out_fourCornerWorldPositions[2] = m_center + i + j;
	out_fourCornerWorldPositions[3] = m_center - i + j;
}

Vec2 const OBB2::GetLocalPosForWorldPos(Vec2 const& worldPos) const
{
	Vec2 displacement = worldPos - m_center;
	return Vec2(DotProduct2D(displacement, m_iBasisNormal), DotProduct2D(displacement, m_iBasisNormal.GetRotated90Degrees()));
}

Vec2 const OBB2::GetWorldPosForLocalPos(Vec2 const& localPos) const
{
	return localPos.x * m_iBasisNormal + localPos.y * m_iBasisNormal.GetRotated90Degrees() + m_center;
}

void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_iBasisNormal.RotateDegrees(rotationDeltaDegrees);
}
