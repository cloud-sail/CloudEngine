#include "Engine/Core/HeatMaps.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/Gradient.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"



TileHeatMap::TileHeatMap(IntVec2 const& dimensions, float initialValue)
	: m_dimensions(dimensions)
{
	int numTiles = GetNumTiles();
	m_values.resize(numTiles);
	SetAllValues(initialValue);
}

bool TileHeatMap::IsInBounds(int tileIndex) const
{
	return tileIndex >= 0 && tileIndex < (m_dimensions.x * m_dimensions.y);
}

bool TileHeatMap::IsInBounds(IntVec2 const& tileCoords) const
{
	return	(tileCoords.x >= 0) && (tileCoords.x < m_dimensions.x) &&
			(tileCoords.y >= 0) && (tileCoords.y < m_dimensions.y);
}

int TileHeatMap::GetNumTiles() const
{
	return m_dimensions.x * m_dimensions.y;
}

int TileHeatMap::GetTileIndexForCoords(IntVec2 const& tileCoords) const
{
	return tileCoords.x + tileCoords.y * m_dimensions.x;
}


float TileHeatMap::GetValueAtIndex(int tileIndex) const
{
	GUARANTEE_OR_DIE(IsInBounds(tileIndex), "Invalid TileIndex in TileHeatMap");
	return m_values[tileIndex];
}

float TileHeatMap::GetValueAtCoords(IntVec2 tileCoords) const
{
	GUARANTEE_OR_DIE(IsInBounds(tileCoords), "Invalid TileCoords in TileHeatMap");
	int tileIndex = GetTileIndexForCoords(tileCoords);
	return m_values[tileIndex];
}

void TileHeatMap::SetAllValues(float value)
{
	int numTiles = GetNumTiles();
	for (int i = 0; i < numTiles; ++i)
	{
		m_values[i] = value;
	}
}

void TileHeatMap::SetValueAtIndex(int tileIndex, float value)
{
	GUARANTEE_OR_DIE(IsInBounds(tileIndex), "Invalid TileIndex in TileHeatMap");
	m_values[tileIndex] = value;
}

void TileHeatMap::SetValueAtCoords(IntVec2 tileCoords, float value)
{
	GUARANTEE_OR_DIE(IsInBounds(tileCoords), "Invalid TileCoords in TileHeatMap");
	int tileIndex = GetTileIndexForCoords(tileCoords);
	m_values[tileIndex] = value;
}

void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, FloatRange valueRange, Rgba8 lowColor, Rgba8 highColor, float specialValue, Rgba8 specialColor) const
{
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = tileX + m_dimensions.x * tileY;
			float value = m_values[tileIndex];
			Rgba8 color;
			if (value == specialValue)
			{
				color = specialColor;
			}
			else
			{
				float fractionWithInRange = GetFractionWithinRange(value, valueRange.m_min, valueRange.m_max);
				fractionWithInRange = GetClampedZeroToOne(fractionWithInRange);
				color = Interpolate(lowColor, highColor, fractionWithInRange);
			}

			float outMinX = RangeMap(static_cast<float>(tileX), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMaxX = RangeMap(static_cast<float>(tileX + 1), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMinY = RangeMap(static_cast<float>(tileY), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);
			float outMaxY = RangeMap(static_cast<float>(tileY + 1), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);

			AABB2 tileBounds = AABB2(outMinX, outMinY, outMaxX, outMaxY);
			AddVertsForAABB2D(verts, tileBounds, color);
		}
	}
}


void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, Gradient const& colorGradient, FloatRange valueRange /*= FloatRange::ZERO_TO_ONE*/, float specialValue /*= 999999.f*/, Rgba8 specialColor /*= Rgba8(255, 0, 255)*/) const
{
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = tileX + m_dimensions.x * tileY;
			float value = m_values[tileIndex];
			Rgba8 color;
			if (value == specialValue)
			{
				color = specialColor;
			}
			else
			{
				float fractionWithInRange = GetFractionWithinRange(value, valueRange.m_min, valueRange.m_max);
				fractionWithInRange = GetClampedZeroToOne(fractionWithInRange);
				color = colorGradient.Evaluate(fractionWithInRange);
				//color = Interpolate(lowColor, highColor, fractionWithInRange);
			}

			float outMinX = RangeMap(static_cast<float>(tileX), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMaxX = RangeMap(static_cast<float>(tileX + 1), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMinY = RangeMap(static_cast<float>(tileY), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);
			float outMaxY = RangeMap(static_cast<float>(tileY + 1), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);

			AABB2 tileBounds = AABB2(outMinX, outMinY, outMaxX, outMaxY);
			AddVertsForAABB2D(verts, tileBounds, color);
		}
	}
}

void TileHeatMap::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, FloatRange valueRange /*= FloatRange::ZERO_TO_ONE*/, float midValue /*= 0.5f*/, Rgba8 lowColor /*= Rgba8(0, 0, 0, 100)*/, Rgba8 midLowColor /*= Rgba8(0, 0, 0, 100)*/, Rgba8 midHighColor /*= Rgba8(0, 0, 0, 100)*/, Rgba8 highColor /*= Rgba8(255, 255, 255, 100)*/, float specialValue /*= 999999.f*/, Rgba8 specialColor /*= Rgba8(255, 0, 255)*/) const
{
	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = tileX + m_dimensions.x * tileY;
			float value = m_values[tileIndex];
			Rgba8 color;
			if (value == specialValue)
			{
				color = specialColor;
			}
			else
			{
				float fractionWithInRange = RangeMapClamped(value, valueRange.m_min, valueRange.m_max, 0.f, 1.f);
				float midFraction = RangeMapClamped(midValue, valueRange.m_min, valueRange.m_max, 0.f, 1.f);

				if (fractionWithInRange >= midFraction)
				{
					float highFraction = RangeMapClamped(fractionWithInRange, midFraction, 1.f, 0.f, 1.f);
					color = Interpolate(midHighColor, highColor, highFraction);

				}
				else
				{
					float lowFraction = RangeMapClamped(fractionWithInRange, 0.f, midFraction, 0.f, 1.f);
					color = Interpolate(lowColor, midLowColor, lowFraction);
				}
			}

			float outMinX = RangeMap(static_cast<float>(tileX), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMaxX = RangeMap(static_cast<float>(tileX + 1), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMinY = RangeMap(static_cast<float>(tileY), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);
			float outMaxY = RangeMap(static_cast<float>(tileY + 1), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);

			AABB2 tileBounds = AABB2(outMinX, outMinY, outMaxX, outMaxY);
			AddVertsForAABB2D(verts, tileBounds, color);
		}
	}
}

FloatRange TileHeatMap::GetRangeOffValuesExcludingSpecial(float specialValueToIgnore) const
{
	FloatRange rangeOfSpecialValues(FLT_MAX, -FLT_MAX);
	for (int index = 0; index < (int)m_values.size(); ++index)
	{
		float value = m_values[index];
		if (value != specialValueToIgnore)
		{
			rangeOfSpecialValues.StretchToIncludeValue(value);
		}
	}

	return rangeOfSpecialValues;


	//-----------------------------------------------------------------------------------------------
	//float currentMax = -FLT_MAX;
	//float currentMin = FLT_MAX;
	//for (int i = 0; i < size; ++i) {
	//	if (data[i] > currentMax) {
	//		currentMax = data[i];
	//	}
	//	if (data[i] < currentMin) {
	//		currentMin = data[i];
	//	}
	//}
}

//-----------------------------------------------------------------------------------------------
TileVectorField::TileVectorField(IntVec2 const& dimensions, Vec2 initialValue /*= Vec2::ZERO*/)
	: m_dimensions(dimensions)
{
	int numTiles = GetNumTiles();
	m_values.resize(numTiles);
	SetAllValues(initialValue);
}

bool TileVectorField::IsInBounds(int tileIndex) const
{
	return tileIndex >= 0 && tileIndex < (m_dimensions.x * m_dimensions.y);
}

bool TileVectorField::IsInBounds(IntVec2 const& tileCoords) const
{
	return	(tileCoords.x >= 0) && (tileCoords.x < m_dimensions.x) &&
			(tileCoords.y >= 0) && (tileCoords.y < m_dimensions.y);
}

int TileVectorField::GetNumTiles() const
{
	return m_dimensions.x * m_dimensions.y;
}

int TileVectorField::GetTileIndexForCoords(IntVec2 const& tileCoords) const
{
	GUARANTEE_OR_DIE(IsInBounds(tileCoords), "Invalid TileCoords in TileFlowField");
	return tileCoords.x + tileCoords.y * m_dimensions.x;
}

Vec2 TileVectorField::GetValueAtIndex(int tileIndex) const
{
	GUARANTEE_OR_DIE(IsInBounds(tileIndex), "Invalid TileIndex in TileFlowField");
	return m_values[tileIndex];
}

Vec2 TileVectorField::GetValueAtCoords(IntVec2 tileCoords) const
{
	GUARANTEE_OR_DIE(IsInBounds(tileCoords), "Invalid TileCoords in TileFlowField");
	int tileIndex = GetTileIndexForCoords(tileCoords);
	return m_values[tileIndex];
}

void TileVectorField::SetAllValues(Vec2 const& value)
{
	int numTiles = GetNumTiles();
	for (int i = 0; i < numTiles; ++i)
	{
		m_values[i] = value;
	}
}

void TileVectorField::SetValueAtIndex(int tileIndex, Vec2 const& value)
{
	GUARANTEE_OR_DIE(IsInBounds(tileIndex), "Invalid TileIndex in TileFlowField");
	m_values[tileIndex] = value;
}

void TileVectorField::SetValueAtCoords(IntVec2 tileCoords, Vec2 const& value)
{
	GUARANTEE_OR_DIE(IsInBounds(tileCoords), "Invalid TileCoords in TileFlowField");
	int tileIndex = GetTileIndexForCoords(tileCoords);
	m_values[tileIndex] = value;
}

void TileVectorField::AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, float thicknessRatioToCell /*= 0.1f*/, Rgba8 color /*= Rgba8::RED*/) const
{
	Vec2 totalDimensions = totalBounds.GetDimensions();
	Vec2 cellDimensions = Vec2(totalDimensions.x / static_cast<float>(m_dimensions.x), totalDimensions.y / static_cast<float>(m_dimensions.y));
	Vec2 halfCellDimensions = 0.5f * cellDimensions;

	float cellSize = (cellDimensions.x > cellDimensions.y) ? cellDimensions.y : cellDimensions.x;
	float thickness = cellSize * thicknessRatioToCell;

	float lengthRatio = (1.f - thicknessRatioToCell);

	for (int tileY = 0; tileY < m_dimensions.y; ++tileY)
	{
		for (int tileX = 0; tileX < m_dimensions.x; ++tileX)
		{
			int tileIndex = tileX + m_dimensions.x * tileY;
			Vec2 value = m_values[tileIndex];

			Vec2 endOffset = value * halfCellDimensions * lengthRatio; // ellipse and keep the line in the cell

			float outMinX = RangeMap(static_cast<float>(tileX), 0.f, static_cast<float>(m_dimensions.x), totalBounds.m_mins.x, totalBounds.m_maxs.x);
			float outMinY = RangeMap(static_cast<float>(tileY), 0.f, static_cast<float>(m_dimensions.y), totalBounds.m_mins.y, totalBounds.m_maxs.y);

			Vec2 startPos = Vec2(outMinX, outMinY) + halfCellDimensions;

			AddVertsForDisc2D(verts, startPos, thickness * 1.5f, color, 4);
			AddVertsForLineSegment2D(verts, startPos, startPos + endOffset, thickness, color);
		}
	}
}
