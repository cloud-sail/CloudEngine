#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/MathUtils.hpp"

//-----------------------------------------------------------------------------------------------
const Rgba8 Rgba8::RED{ 255,0,0,255 };
const Rgba8 Rgba8::GREEN{ 0,255,0,255 };
const Rgba8 Rgba8::BLUE{ 0,0,255,255 };
const Rgba8 Rgba8::YELLOW{ 255,255,0,255 };
const Rgba8 Rgba8::MAGENTA{ 255,0,255,255 };
const Rgba8 Rgba8::CYAN{ 0,255,255,255 };
const Rgba8 Rgba8::OPAQUE_WHITE{ 255,255,255,255 };
const Rgba8 Rgba8::TRANSPARENT_BLACK{ 0,0,0,0 };


Rgba8 const Rgba8::MakeFromWaveLength(float w)
{
	w = GetClamped(w, 380.f, 780.f);
	float R, G, B;

	if (w >= 380.0f && w < 440.0f)
	{
		R = -(w - 440.0f) / (440.0f - 380.0f);
		G = 0.0f;
		B = 1.0f;
	}
	else if (w >= 440.0f && w < 490.0f)
	{
		R = 0.0f;
		G = (w - 440.0f) / (490.0f - 440.0f);
		B = 1.0f;
	}
	else if (w >= 490.0f && w < 510.0f)
	{
		R = 0.0f;
		G = 1.0f;
		B = -(w - 510.0f) / (510.0f - 490.0f);
	}
	else if (w >= 510.0f && w < 580.0f)
	{
		R = (w - 510.0f) / (580.0f - 510.0f);
		G = 1.0f;
		B = 0.0f;
	}
	else if (w >= 580.0f && w < 645.0f)
	{
		R = 1.0f;
		G = -(w - 645.0f) / (645.0f - 580.0f);
		B = 0.0f;
	}
	else if (w >= 645.0f && w <= 780.0f)
	{
		R = 1.0f;
		G = 0.0f;
		B = 0.0f;
	}
	else
	{
		R = 0.0f;
		G = 0.0f;
		B = 0.0f;
	}

	R = GetClampedZeroToOne(R);
	G = GetClampedZeroToOne(G);
	B = GetClampedZeroToOne(B);

	Rgba8 color;
	color.r = DenormalizeByte(R);
	color.g = DenormalizeByte(G);
	color.b = DenormalizeByte(B);

	return color;
}

Rgba8 const Rgba8::MakeFromZeroToOne(float zeroToOne)
{
	float wavelength = Interpolate(380.f, 780.f, zeroToOne);
	return MakeFromWaveLength(wavelength);
}

//-----------------------------------------------------------------------------------------------
Rgba8::Rgba8(Rgba8 const& copy)
	: r(copy.r)
	, g(copy.g)
	, b(copy.b)
	, a(copy.a)
{
}

Rgba8::Rgba8(unsigned char redByte, unsigned char greenByte, unsigned char blueByte, unsigned char alphaByte)
	: r(redByte)
	, g(greenByte)
	, b(blueByte)
	, a(alphaByte)
{
}

void Rgba8::GetAsFloats(float* colorAsFloats) const
{
	colorAsFloats[0] = static_cast<float>(r) / 255.f;
	colorAsFloats[1] = static_cast<float>(g) / 255.f;
	colorAsFloats[2] = static_cast<float>(b) / 255.f;
	colorAsFloats[3] = static_cast<float>(a) / 255.f;
}

void Rgba8::SetFromText(char const* text)
{
	Strings tokens = SplitStringOnDelimiter(text, ',');
	if (tokens.size() != 3 && tokens.size() != 4)
	{
		ERROR_AND_DIE(Stringf("Wrong text format for Rgba8! text: %s", text));
	}
	
	int rValue = atoi(tokens[0].c_str());
	int gValue = atoi(tokens[1].c_str());
	int bValue = atoi(tokens[2].c_str());

	GUARANTEE_OR_DIE(rValue >= 0 && rValue <= 255, Stringf("Wrong value for Rgba8! text: %s", text));
	GUARANTEE_OR_DIE(gValue >= 0 && gValue <= 255, Stringf("Wrong value for Rgba8! text: %s", text));
	GUARANTEE_OR_DIE(bValue >= 0 && bValue <= 255, Stringf("Wrong value for Rgba8! text: %s", text));

	r = static_cast<unsigned char>(rValue);
	g = static_cast<unsigned char>(gValue);
	b = static_cast<unsigned char>(bValue);

	a = 255;
	if (tokens.size() == 4)
	{
		int aValue = atoi(tokens[3].c_str());
		GUARANTEE_OR_DIE(aValue >= 0 && aValue <= 255, Stringf("Wrong value for Rgba8! text: %s", text));
		a = static_cast<unsigned char>(aValue);
	}
}

void Rgba8::ScaleRGB(float rgbScale)
{
	float floatR = NormalizeByte(r) * rgbScale;
	float floatG = NormalizeByte(g) * rgbScale;
	float floatB = NormalizeByte(b) * rgbScale;

	r = DenormalizeByte(floatR);
	g = DenormalizeByte(floatG);
	b = DenormalizeByte(floatB);
}

void Rgba8::ScaleAlpha(float alphaScale)
{
	float floatA = NormalizeByte(b) * alphaScale;

	a = DenormalizeByte(floatA);
}

bool Rgba8::operator==(Rgba8 const& compare) const
{
	return r == compare.r && g == compare.g && b == compare.b && a == compare.a;
}

Rgba8 Interpolate(Rgba8 start, Rgba8 end, float fractionOfEnd)
{
	float r = Interpolate(NormalizeByte(start.r), NormalizeByte(end.r), fractionOfEnd);
	float g = Interpolate(NormalizeByte(start.g), NormalizeByte(end.g), fractionOfEnd);
	float b = Interpolate(NormalizeByte(start.b), NormalizeByte(end.b), fractionOfEnd);
	float a = Interpolate(NormalizeByte(start.a), NormalizeByte(end.a), fractionOfEnd);
	return Rgba8(DenormalizeByte(r), DenormalizeByte(g), DenormalizeByte(b), DenormalizeByte(a));
}
