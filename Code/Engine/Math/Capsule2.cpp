#include "Engine/Math/Capsule2.hpp"

Capsule2::Capsule2(LineSegment2 const& bone, float radius)
	: m_bone(bone)
	, m_radius(radius)
{
}

Capsule2::Capsule2(Vec2 const& boneStart, Vec2 const& boneEnd, float radius)
	: m_bone(LineSegment2(boneStart, boneEnd))
	, m_radius(radius)
{
}

void Capsule2::Translate(Vec2 translation)
{
	m_bone.Translate(translation);
}

void Capsule2::SetCenter(Vec2 newCenter)
{
	m_bone.SetCenter(newCenter);
}

void Capsule2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_bone.RotateAboutCenter(rotationDeltaDegrees);
}


