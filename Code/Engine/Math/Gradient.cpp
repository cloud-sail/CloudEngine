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

Gradient Gradient::MakeSkyGradient()
{
	std::vector<GradientRgba8Key> keys =
	{
		GradientRgba8Key(0.0f,      Rgba8(10, 15, 30)),      // 0:00 Midnight - Deep blue black
		GradientRgba8Key(0.208f,    Rgba8(60, 70, 100)),     // 5:00 Dawn
		GradientRgba8Key(0.25f,     Rgba8(255, 150, 100)),   // 6:00 Sunrise - Orange red
		GradientRgba8Key(0.333f,    Rgba8(135, 206, 235)),   // 8:00 Morning - Sky blue
		GradientRgba8Key(0.5f,      Rgba8(80, 150, 255)),    // 12:00 Noon - Pure blue
		GradientRgba8Key(0.75f,     Rgba8(255, 180, 120)),   // 18:00 Sunset - Orange
		GradientRgba8Key(0.833f,    Rgba8(100, 60, 80)),     // 20:00 Dusk - Purple red
		GradientRgba8Key(0.917f,    Rgba8(20, 25, 50)),      // 22:00 Night - Deep blue
		GradientRgba8Key(1.0f,      Rgba8(10, 15, 30)),      // 24:00 Midnight - Same as 0:00


/*		GradientRgba8Key(0.0f,      Rgba8(10, 15, 30)),       
		GradientRgba8Key(0.208f,    Rgba8(60, 80, 120)),      
		GradientRgba8Key(0.25f,     Rgba8(255, 200, 150)),    
		GradientRgba8Key(0.333f,    Rgba8(135, 206, 235)),    
		GradientRgba8Key(0.5f,      Rgba8(135, 206, 250)),    
		GradientRgba8Key(0.75f,     Rgba8(255, 160, 100)),    
		GradientRgba8Key(0.833f,    Rgba8(40, 50, 90)),       
		GradientRgba8Key(0.917f,    Rgba8(15, 20, 40)),       
		GradientRgba8Key(1.0f,      Rgba8(10, 15, 30)),  */     
	};

	Gradient result;
	result.SetKeys(keys);
	return result;
}

Gradient Gradient::MakeSkyAmbientLightGradient()
{
	std::vector<GradientRgba8Key> keys =
	{
		// Midnight 0:00 - Deep blue glow (moonlight)
		GradientRgba8Key(0.0f,      Rgba8(5, 10, 25, 255)),

		// Pre-dawn 4:00 - Very dark blue-purple
		GradientRgba8Key(0.167f,    Rgba8(15, 20, 40, 255)),

		// Before sunrise 5:30 - Gradually brightening blue
		GradientRgba8Key(0.229f,    Rgba8(50, 70, 110, 255)),

		// Sunrise 6:00 - Warm orange-pink (dawn)
		GradientRgba8Key(0.25f,     Rgba8(255, 180, 150, 255)),

		// Early morning 7:30 - Soft golden
		GradientRgba8Key(0.313f,    Rgba8(255, 230, 200, 255)),

		// Morning 9:00 - Close to white light
		GradientRgba8Key(0.375f,    Rgba8(255, 250, 240, 255)),

		// Noon 12:00 - Pure white strong light
		GradientRgba8Key(0.5f,      Rgba8(255, 255, 255, 255)),

		// Afternoon 15:00 - Still bright white light
		GradientRgba8Key(0.625f,    Rgba8(255, 250, 240, 255)),

		// Evening 17:00 - Beginning to turn golden
		GradientRgba8Key(0.708f,    Rgba8(255, 230, 200, 255)),

		// Sunset 18:00 - Warm orange-red (dusk)
		GradientRgba8Key(0.75f,     Rgba8(255, 160, 100, 255)),

		// After sunset 19:00 - Deep orange turning purple
		GradientRgba8Key(0.792f,    Rgba8(180, 100, 140, 255)),

		// Twilight 20:00 - Dark blue-purple
		GradientRgba8Key(0.833f,    Rgba8(60, 50, 90, 255)),

		// Night 21:30 - Deep blue
		GradientRgba8Key(0.896f,    Rgba8(20, 25, 50, 255)),

		// Late night 23:00 - Back to midnight color
		GradientRgba8Key(0.958f,    Rgba8(8, 12, 28, 255)),

		// Midnight 24:00 - Same as 0:00 (loop)
		GradientRgba8Key(1.0f,      Rgba8(5, 10, 25, 255)),
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
