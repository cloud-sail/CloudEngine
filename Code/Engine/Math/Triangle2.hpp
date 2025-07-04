#pragma once
#include "Engine/Math/Vec2.hpp"


struct Triangle2
{
public:
	Vec2 m_pointsCounterClockwise[3] = {};

public:
	Triangle2(Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2);
	~Triangle2() = default;

	void Translate(Vec2 translation);
};

