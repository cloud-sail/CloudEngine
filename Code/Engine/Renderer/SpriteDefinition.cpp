#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Texture.hpp"



SpriteDefinition::SpriteDefinition(SpriteSheet const& spriteSheet, int spriteIndex, Vec2 const& uvAtMins, Vec2 const& uvAtMaxs)
	: m_spriteSheet(spriteSheet)
	, m_spriteIndex(spriteIndex)
	, m_uvAtMins(uvAtMins)
	, m_uvAtMaxs(uvAtMaxs)
{
}

void SpriteDefinition::GetUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs) const
{
	out_uvAtMins = m_uvAtMins;
	out_uvAtMaxs = m_uvAtMaxs;
}

AABB2 SpriteDefinition::GetUVs() const
{
	return AABB2(m_uvAtMins, m_uvAtMaxs);
}

SpriteSheet const& SpriteDefinition::GetSpriteSheet() const
{
	return m_spriteSheet;
}

Texture& SpriteDefinition::GetTexture() const
{
	return m_spriteSheet.GetTexture();
}

float SpriteDefinition::GetAspect() const
{
	IntVec2 textureDimensions = m_spriteSheet.GetTexture().GetDimensions();
	Vec2 uvDimensions = m_uvAtMaxs - m_uvAtMins;
	float width = static_cast<float>(textureDimensions.x) * uvDimensions.x;
	float height = static_cast<float>(textureDimensions.y) * uvDimensions.y;

	return width / height;
}
