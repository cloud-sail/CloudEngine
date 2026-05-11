#include "Engine/Math/ConvexPoly2.hpp"
#include "Engine/Math/MathUtils.hpp"

void ConvexPoly2::RotateAroundPoint(Vec2 const& point, float deltaDegrees)
{
	float cosDelta = CosDegrees(deltaDegrees);
	float sinDelta = SinDegrees(deltaDegrees);

	for (int vertexIndex = 0; vertexIndex < (int)m_vertexPositionsCCW.size(); ++vertexIndex)
	{
		Vec2 displacement = m_vertexPositionsCCW[vertexIndex] - point;
		Vec2 rotatedDisplacement = Vec2(cosDelta * displacement.x - sinDelta * displacement.y, sinDelta * displacement.x + cosDelta * displacement.y);
		m_vertexPositionsCCW[vertexIndex] = point + rotatedDisplacement;
	}
}

void ConvexPoly2::ScaleAroundPoint(Vec2 const& point, float scalingFactor)
{
	for (int vertexIndex = 0; vertexIndex < (int)m_vertexPositionsCCW.size(); ++vertexIndex)
	{
		Vec2 displacement = m_vertexPositionsCCW[vertexIndex] - point;
		Vec2 scaledDisplacement = displacement * scalingFactor;
		m_vertexPositionsCCW[vertexIndex] = point + scaledDisplacement;
	}
}

void ConvexPoly2::Translate(Vec2 const& translation)
{
	for (int vertexIndex = 0; vertexIndex < (int)m_vertexPositionsCCW.size(); ++vertexIndex)
	{
		m_vertexPositionsCCW[vertexIndex] += translation;
	}
}
