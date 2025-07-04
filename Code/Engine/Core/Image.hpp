#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <string>
#include <vector>

//-----------------------------------------------------------------------------------------------
// Only Support channel size = 8 bit
// not support hdr image, it needs to use stbi_loadf

//-----------------------------------------------------------------------------------------------
class Image
{
public:
	Image();
	~Image();
	Image(char const* imageFilePath);
	Image(IntVec2 size, Rgba8 color, char const* imageFilePath = "UNKNOWN");


	IntVec2		GetDimensions() const;
	std::string const& GetImageFilePath() const;
	const void* GetRawData() const;

	bool	IsInBounds(IntVec2 const& tileCoords) const;
	Rgba8	GetTexelColor(IntVec2 const& texelCoords) const;
	void	SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor);


	Image GetBoxBlurred(int blurRadius) const;

private:
	std::string			m_imageFilePath;
	IntVec2			m_dimensions = IntVec2(0, 0);
	std::vector< Rgba8 >		m_rgbaTexels;  // or Rgba8* m_rgbaTexels = nullptr; if you prefer new[] and delete[]
};
