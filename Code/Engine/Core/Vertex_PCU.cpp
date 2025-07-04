#include "Engine/Core/Vertex_PCU.hpp"

Vertex_PCU::Vertex_PCU(Vertex_PCU const& copy)
	: m_position(copy.m_position)
	, m_color(copy.m_color)
	, m_uvTexCoords(copy.m_uvTexCoords)
{}

Vertex_PCU::Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords)
	: m_position(position)
	, m_color(color)
	, m_uvTexCoords(uvTexCoords)
{}
