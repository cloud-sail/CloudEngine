#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/FloatRange.hpp"

#include <vector>

//-----------------------------------------------------------------------------------------------
struct Vertex_PCU;
struct AABB2;
struct Rgba8;
struct FloatRange;

class Gradient;

//-----------------------------------------------------------------------------------------------
class TileHeatMap
{
public:
	TileHeatMap(IntVec2 const& dimensions, float initialValue = 0.f);


	bool IsInBounds(int tileIndex) const;
	bool IsInBounds(IntVec2 const& tileCoords) const;
	int GetNumTiles() const;
	int GetTileIndexForCoords(IntVec2 const& tileCoords) const;

	float GetValueAtIndex(int tileIndex) const;
	float GetValueAtCoords(IntVec2 tileCoords) const;

	void SetAllValues(float value);
	void SetValueAtIndex(int tileIndex, float value);
	void SetValueAtCoords(IntVec2 tileCoords, float value);

	FloatRange GetRangeOffValuesExcludingSpecial(float specialValueToIgnore) const;


	void AddVertsForDebugDraw(	std::vector<Vertex_PCU>& verts, AABB2 totalBounds, FloatRange valueRange = FloatRange::ZERO_TO_ONE, 
								Rgba8 lowColor = Rgba8(0, 0, 0, 100), Rgba8 highColor = Rgba8(255, 255, 255, 100), 
								float specialValue = 999999.f, Rgba8 specialColor = Rgba8(255, 0, 255) ) const;

	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, Gradient const& colorGradient, FloatRange valueRange = FloatRange::ZERO_TO_ONE,
		float specialValue = 999999.f, Rgba8 specialColor = Rgba8(255, 0, 255)) const;

	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, FloatRange valueRange = FloatRange::ZERO_TO_ONE, float midValue = 0.5f,
		Rgba8 lowColor = Rgba8::CYAN, Rgba8 midLowColor = Rgba8::BLUE, Rgba8 midHighColor = Rgba8(50,0,0), Rgba8 highColor = Rgba8::RED,
		float specialValue = 999999.f, Rgba8 specialColor = Rgba8(255, 0, 255)) const;

public:
	std::vector<float> m_values;
	IntVec2 m_dimensions;
};

//-----------------------------------------------------------------------------------------------
class TileVectorField
{
public:
	TileVectorField(IntVec2 const& dimensions, Vec2 initialValue = Vec2::ZERO);

	bool IsInBounds(int tileIndex) const;
	bool IsInBounds(IntVec2 const& tileCoords) const;
	int GetNumTiles() const;
	int GetTileIndexForCoords(IntVec2 const& tileCoords) const;

	Vec2 GetValueAtIndex(int tileIndex) const;
	Vec2 GetValueAtCoords(IntVec2 tileCoords) const;

	void SetAllValues(Vec2 const& value);
	void SetValueAtIndex(int tileIndex, Vec2 const& value);
	void SetValueAtCoords(IntVec2 tileCoords, Vec2 const& value);

	void AddVertsForDebugDraw(std::vector<Vertex_PCU>& verts, AABB2 totalBounds, float thicknessRatioToCell = 0.1f, Rgba8 color = Rgba8::RED) const;

public:
	std::vector<Vec2> m_values;
	IntVec2 m_dimensions;
};



