#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Math/MathUtils.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(SpriteSheet const& sheet, int startSpriteIndex, int endSpriteIndex, float framesPerSecond, SpriteAnimPlaybackType playbackType /*= SpriteAnimPlaybackType::LOOP*/)
	: m_spriteSheet(sheet)
	, m_startSpriteIndex(startSpriteIndex)
	, m_endSpriteIndex(endSpriteIndex)
	, m_framesPerSecond(framesPerSecond)
	, m_playbackType(playbackType)
{
}

SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float seconds) const
{
	// Suppose framesPerSeconds>=0.f, end>start
	// n = floor (t / T) = floor (t * f)
	int currentFrame = RoundDownToInt(seconds * m_framesPerSecond);

	int spriteIndex = 0;
	switch (m_playbackType)
	{
	case SpriteAnimPlaybackType::ONCE:
		spriteIndex = GetSpriteIndexOnce(currentFrame);
		break;
	case SpriteAnimPlaybackType::PINGPONG:
		spriteIndex = GetSpriteIndexPingpong(currentFrame);
		break;
	case SpriteAnimPlaybackType::LOOP:
	default:
		spriteIndex = GetSpriteIndexLoop(currentFrame);
	}

	return m_spriteSheet.GetSpriteDef(spriteIndex);
}

float SpriteAnimDefinition::GetDuration() const
{
	// period frames / fps

	int periodFrames = 0;
	if (m_playbackType == SpriteAnimPlaybackType::ONCE || m_playbackType == SpriteAnimPlaybackType::LOOP)
	{
		periodFrames = m_endSpriteIndex - m_startSpriteIndex + 1;
	}
	else if (m_playbackType == SpriteAnimPlaybackType::PINGPONG)
	{
		periodFrames = 2 * (m_endSpriteIndex - m_startSpriteIndex);
	}

	return static_cast<float>(periodFrames) / m_framesPerSecond;
}

int SpriteAnimDefinition::GetSpriteIndexOnce(int currentFrame) const
{
	//int totalFrame = m_endSpriteIndex - m_startSpriteIndex + 1;
	int lastFrame = m_endSpriteIndex - m_startSpriteIndex;
	int spriteIndexOffset = (currentFrame < lastFrame) ? currentFrame : lastFrame;
	return m_startSpriteIndex + spriteIndexOffset;
}

int SpriteAnimDefinition::GetSpriteIndexLoop(int currentFrame) const
{
	int periodFrames = m_endSpriteIndex - m_startSpriteIndex + 1;
	int spriteIndexOffset = currentFrame % periodFrames;
	return m_startSpriteIndex + spriteIndexOffset;
}

int SpriteAnimDefinition::GetSpriteIndexPingpong(int currentFrame) const
{
	int periodFrames = 2 * (m_endSpriteIndex - m_startSpriteIndex);
	int halfPeriodFrames = (m_endSpriteIndex - m_startSpriteIndex);
	int t = currentFrame % periodFrames;
	int spriteIndexOffset = 0;
	if (t < halfPeriodFrames)
	{
		spriteIndexOffset = t;
	}
	else
	{
		spriteIndexOffset = periodFrames - t;
	}
	return m_startSpriteIndex + spriteIndexOffset;
}
