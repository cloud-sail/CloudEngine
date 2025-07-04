#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/LineSegment2.hpp"

struct Capsule2
{
public:
	LineSegment2 m_bone;
	float m_radius;

public:
	Capsule2() = default;
	~Capsule2() = default;
	explicit Capsule2(LineSegment2 const& bone, float radius);
	explicit Capsule2(Vec2 const& boneStart, Vec2 const& boneEnd, float radius);

	void Translate(Vec2 translation);
	void SetCenter(Vec2 newCenter);
	void RotateAboutCenter(float rotationDeltaDegrees);
};

