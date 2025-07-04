#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"

struct Vertex_PCU
{
public:
	Vec3	m_position;		// P
	Rgba8	m_color;		// C
	Vec2	m_uvTexCoords;	// U

public:
	~Vertex_PCU() {}
	Vertex_PCU() {}
	Vertex_PCU(Vertex_PCU const& copyFrom);
	explicit Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords);
};

