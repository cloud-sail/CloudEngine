#include "Engine/Math/Gradient.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <algorithm>


GradientRgba8Key::GradientRgba8Key(float time, Rgba8 const& color)
	: m_time(time)
	, m_color(color)
{

}

Gradient Gradient::MakeHeatGradient()
{
	std::vector<GradientRgba8Key> keys =
	{
		GradientRgba8Key(0.0f,	Rgba8(16, 0, 116)),
		GradientRgba8Key(0.25f, Rgba8(106, 0, 53)),
		GradientRgba8Key(0.5f,	Rgba8(182, 0, 0)),
		GradientRgba8Key(0.75f, Rgba8(203, 112, 25)),
		GradientRgba8Key(1.0f,	Rgba8(244, 244, 96)),
	};

	Gradient result;
	result.SetKeys(keys);
	return result;
}

void Gradient::SetKeys(const std::vector<GradientRgba8Key>& keys)
{
	m_keys = keys;
	std::sort(m_keys.begin(), m_keys.end(), [](const GradientRgba8Key& a, const GradientRgba8Key& b) {
		return a.m_time < b.m_time;
		});
}

Rgba8 Gradient::Evaluate(float t) const
{
	int numKeys = (int)m_keys.size();
	if (numKeys == 0)
	{
		return Rgba8::OPAQUE_WHITE;
	}
	if (numKeys == 1)
	{
		return m_keys[0].m_color;
	}

	if (t <= m_keys[0].m_time)
	{
		return m_keys[0].m_color;
	}
	if (t >= m_keys[numKeys - 1].m_time)
	{
		return  m_keys[numKeys - 1].m_color;
	}

	for (int i = 0; i < numKeys - 1; ++i) {
		if (t >= m_keys[i].m_time && t <= m_keys[i + 1].m_time) 
		{
			float localT = RangeMapClamped(t, m_keys[i].m_time, m_keys[i + 1].m_time, 0.f, 1.f);
			return Interpolate(m_keys[i].m_color, m_keys[i + 1].m_color, localT);
		}
	}
	return  m_keys[numKeys - 1].m_color;
}
