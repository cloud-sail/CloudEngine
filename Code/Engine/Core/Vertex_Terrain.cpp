#include "Engine/Core/Vertex_Terrain.hpp"
#include "Engine/Math/Quantization.hpp"

Vertex_Terrain::Vertex_Terrain(const Vec3& position, const Vec3& normal, uint8_t matID1 /*= 0*/, uint8_t matID2 /*= 0*/, float blendValue /*= 0.0f*/)
	: m_position(position)
	, m_materialID1(matID1)
	, m_materialID2(matID2)
{
	SetNormal(normal);
	SetBlendValue(blendValue);
}


Vertex_Terrain::Vertex_Terrain(const Vec3& position, const uint8_t smoothNormal[2], uint8_t matID1 /*= 0*/, uint8_t matID2 /*= 0*/, uint8_t blendValue /*= 0*/)
	: m_position(position)
	, m_materialID1(matID1)
	, m_materialID2(matID2)
	, m_blendValue(blendValue)
{
	m_smoothNormal[0] = smoothNormal[0];
	m_smoothNormal[1] = smoothNormal[1];
}

void Vertex_Terrain::SetNormal(const Vec3& normal)
{
	Quantization::OctEncodeNormal(normal, m_smoothNormal[0], m_smoothNormal[1]);
}

void Vertex_Terrain::SetBlendValue(float zeroToOne)
{
	m_blendValue = Quantization::ToUint8FromUNorm(zeroToOne);
}
