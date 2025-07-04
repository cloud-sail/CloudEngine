#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Rgba8.hpp"

struct Vertex_PCUTBN
{
public:
	Vec3	m_position;		// P
	Rgba8	m_color;		// C
	Vec2	m_uvTexCoords;	// U
	Vec3	m_tangent;		// T
	Vec3	m_bitangent;		// B
	Vec3	m_normal;		// N

public:
	~Vertex_PCUTBN() {}
	Vertex_PCUTBN() {}


	explicit Vertex_PCUTBN(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords,
		Vec3 const& tangent, Vec3 const& bitangent, Vec3 const& normal);
};

