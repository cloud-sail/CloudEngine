#include "Engine/Core/Vertex_PNMD.hpp"

Vertex_PNMD::Vertex_PNMD(Vec3 const& position, Vec3 const& normal, uint8_t materialID, uint8_t density)
	: m_position(position)
	, m_normal(normal)
	, m_materialID(materialID)
	, m_density(density)
{

}
