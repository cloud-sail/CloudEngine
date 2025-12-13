#pragma once
#include "Engine/Math/Vec3.hpp"
#include <cstdint>

struct Vertex_Terrain
{
public:
	Vec3 m_position;					// 12 bytes
	uint8_t m_smoothNormal[2] = {0, 0};	// 2 bytes	original: Vec3 12 bytes
	uint8_t m_materialID1 = 0;			// 1 byte
	uint8_t m_materialID2 = 0;			// 1 byte
	uint8_t m_blendValue = 0;			// 1 byte (0.f ~ 1.f)

public:
	~Vertex_Terrain() {}
	Vertex_Terrain() {}

	explicit Vertex_Terrain(const Vec3& position,
		const Vec3& normal,
		uint8_t matID1 = 0,
		uint8_t matID2 = 0,
		float blendValue = 0.0f);

	explicit Vertex_Terrain(const Vec3& position,
		const uint8_t smoothNormal[2],
		uint8_t matID1 = 0,
		uint8_t matID2 = 0,
		uint8_t blendValue = 0);

	void SetNormal(const Vec3& normal); 

	void SetBlendValue(float value);
};
