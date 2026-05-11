#include "Engine/Math/ConvexHull2.hpp"
#include "Engine/Math/ConvexPoly2.hpp"

ConvexHull2::ConvexHull2(ConvexPoly2 const& convexPoly)
{
	int const vertexNum = (int)convexPoly.m_vertexPositionsCCW.size();

	m_boundingPlanes.clear();
	m_boundingPlanes.reserve(vertexNum);

	for (int vertexIndex = 0; vertexIndex < vertexNum; ++vertexIndex)
	{
		Vec2 start = convexPoly.m_vertexPositionsCCW[vertexIndex];
		Vec2 end = convexPoly.m_vertexPositionsCCW[(vertexIndex + 1) % vertexNum];

		Vec2 startToEnd = end - start;
		Vec2 normal = startToEnd.GetRotatedMinus90Degrees();
		normal.Normalize();

		Plane2 plane(normal, start);
		m_boundingPlanes.push_back(plane);
	}
}
