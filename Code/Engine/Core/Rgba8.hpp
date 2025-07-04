#pragma once

//-----------------------------------------------------------------------------------------------
struct Rgba8
{
public:
	// 0 0 0 0 is transparent black
	// 255 255 255 255 is opaque white
	unsigned char r = 255;
	unsigned char g = 255;
	unsigned char b = 255;
	unsigned char a = 255;

	static const Rgba8 RED;
	static const Rgba8 GREEN;
	static const Rgba8 BLUE;
	static const Rgba8 YELLOW;
	static const Rgba8 MAGENTA;
	static const Rgba8 CYAN;
	static const Rgba8 OPAQUE_WHITE;
	static const Rgba8 TRANSPARENT_BLACK;
	static const Rgba8 MakeFromWaveLength(float waveLength);
	static const Rgba8 MakeFromZeroToOne(float zeroToOne);

public:
	~Rgba8() {}
	Rgba8() {}
	Rgba8(Rgba8 const& copyFrom);
	explicit Rgba8(unsigned char redByte, unsigned char greenByte, unsigned char blueByte, unsigned char alphaByte = 255);

	void GetAsFloats(float* colorAsFloats) const;

	// Mutator
	void SetFromText(char const* text);
	void ScaleRGB(float rgbScale);
	void ScaleAlpha(float alphaScale);

	// Operators (const)
	bool		operator==(Rgba8 const& compare) const;
};

//-----------------------------------------------------------------------------------------------
Rgba8 Interpolate(Rgba8 start, Rgba8 end, float fractionOfEnd);
