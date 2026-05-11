#pragma once
#include "Engine/Math/Vec2.hpp"
#include <vector>

struct ConvexPoly2
{
public:
	~ConvexPoly2() = default;
	ConvexPoly2() = default;

	void RotateAroundPoint(Vec2 const& point, float deltaDegrees);
	void ScaleAroundPoint(Vec2 const& point, float scalingFactor);
	void Translate(Vec2 const& translation);

public:
	std::vector<Vec2> m_vertexPositionsCCW;
};

