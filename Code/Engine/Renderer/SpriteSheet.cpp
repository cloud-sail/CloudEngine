#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"

SpriteSheet::SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayout)
	: m_texture(texture)
{
	m_spriteDefs.reserve(simpleGridLayout.x * simpleGridLayout.y);
	float spriteX = 1.f / simpleGridLayout.x;
	float spriteY = 1.f / simpleGridLayout.y;

	constexpr float NUDGED_RATIO = 1.f / 128.f;
	Vec2 nudgedDisp(spriteX * NUDGED_RATIO, spriteY * NUDGED_RATIO);

	for (int y = 0; y < simpleGridLayout.y; ++y)
	{
		for (int x = 0; x < simpleGridLayout.x; ++x)
		{
			int spriteIndex = y * simpleGridLayout.x + x;
			Vec2 uvMins = Vec2(spriteX * static_cast<float>(x), 1.f - spriteY * static_cast<float>(y + 1));
			Vec2 uvMaxs = Vec2(spriteX * static_cast<float>(x + 1), 1.f - spriteY * static_cast<float>(y));
			uvMins += nudgedDisp;
			uvMaxs -= nudgedDisp;
			m_spriteDefs.emplace_back(SpriteDefinition(*this, spriteIndex, uvMins, uvMaxs));
		}
	}

}

Texture& SpriteSheet::GetTexture() const
{
	return m_texture;
}

int SpriteSheet::GetNumSprites() const
{
	return static_cast<int>(m_spriteDefs.size());
}

SpriteDefinition const& SpriteSheet::GetSpriteDef(int spriteIndex) const
{
	return m_spriteDefs[spriteIndex];
}

void SpriteSheet::GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const
{
	m_spriteDefs[spriteIndex].GetUVs(out_uvAtMins, out_uvAtMaxs);
}

AABB2 SpriteSheet::GetSpriteUVs(int spriteIndex) const
{
	return m_spriteDefs[spriteIndex].GetUVs();
}
