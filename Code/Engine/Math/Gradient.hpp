#pragma once
#include "Engine/Core/Rgba8.hpp"
#include <vector>

// time is 0~1
struct GradientRgba8Key
{
	float m_time = 0.f;
	Rgba8 m_color;

	GradientRgba8Key(float time, Rgba8 const& color);
};

class Gradient
{
public:
	Gradient() = default;
	static Gradient MakeHeatGradient();

	void SetKeys(const std::vector<GradientRgba8Key>& keys);

	Rgba8 Evaluate(float t) const;

private:
	std::vector<GradientRgba8Key> m_keys;
};

