#pragma once
#include "Engine/Math/Plane2.hpp"
#include <vector>

struct ConvexPoly2;

struct ConvexHull2
{
public:
	~ConvexHull2() = default;
	ConvexHull2() = default;

	ConvexHull2(ConvexPoly2 const& convexPoly);

public:
	std::vector<Plane2> m_boundingPlanes;
};

