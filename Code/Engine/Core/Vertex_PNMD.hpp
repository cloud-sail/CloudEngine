#pragma once
#include "Engine/Math/Vec3.hpp"
#include <stdint.h>

struct Vertex_PNMD
{
public:
	Vec3	m_position;			// P
	Vec3	m_normal;			// N
	uint8_t m_materialID = 0;	// M
	uint8_t m_density = 0;		// D

public:
	~Vertex_PNMD() {}
	Vertex_PNMD() {}


	explicit Vertex_PNMD(Vec3 const& position, Vec3 const& normal, uint8_t materialID, uint8_t density);

};

