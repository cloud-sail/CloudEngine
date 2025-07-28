#include "Engine/Core/Image.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineCommon.hpp"

#define STB_IMAGE_IMPLEMENTATION // Exactly one .CPP (this Image.cpp) should #define this before #including stb_image.h
#include "ThirdParty/stb/stb_image.h"

Image::Image()
{

}

Image::Image(IntVec2 size, Rgba8 color, char const* imageFilePath /*= "UNKNOWN"*/)
	: m_dimensions(size)
	, m_imageFilePath(imageFilePath)
{
	int numTexels = m_dimensions.x * m_dimensions.y;
	m_rgbaTexels = std::vector<Rgba8>(numTexels, color);
}

Image::~Image()
{

}

Image::Image(char const* imageFilePath, bool flipVertically)
	:m_imageFilePath(imageFilePath)
{
	IntVec2 dimensions = IntVec2::ZERO;		// This will be filled in for us to indicate image width & height
	int bytesPerTexel = 0;					// ...and how many color components the image had (e.g. 3=RGB=24bit, 4=RGBA=32bit)

	// Load (and decompress) the image RGB(A) bytes from a file on disk into a memory buffer (array of bytes)
	if (flipVertically)
	{
		stbi_set_flip_vertically_on_load(1); // We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	}
	else
	{
		stbi_set_flip_vertically_on_load(0); // DirectX: For Texture Cube
	}
	unsigned char* texelData = stbi_load(imageFilePath, &dimensions.x, &dimensions.y, &bytesPerTexel, 0);

	// Check if the load was successful
	GUARANTEE_OR_DIE(texelData, Stringf("Failed to load image \"%s\"", imageFilePath));
	GUARANTEE_OR_DIE(bytesPerTexel >= 3 && bytesPerTexel <= 4, Stringf("CreateTextureFromData failed for \"%s\" - unsupported BPP=%i (must be 3 or 4)", imageFilePath, bytesPerTexel));
	GUARANTEE_OR_DIE(dimensions.x > 0 && dimensions.y > 0, Stringf("CreateTextureFromData failed for \"%s\" - illegal texture dimensions (%i x %i)", imageFilePath, dimensions.x, dimensions.y));

	// Initialize
	m_dimensions = dimensions;

	int numTexels = m_dimensions.x * m_dimensions.y;
	m_rgbaTexels.reserve(numTexels);

	for (int i = 0; i < numTexels; ++i)
	{
		int offset = i * bytesPerTexel;
		Rgba8 color;
		color.r = texelData[offset];
		color.g = texelData[offset + 1];
		color.b = texelData[offset + 2];
		if (bytesPerTexel == 4)
		{
			color.a = texelData[offset + 3];
		}
		m_rgbaTexels.push_back(color);
	}

	stbi_image_free(texelData);
}


std::string const& Image::GetImageFilePath() const
{
	return m_imageFilePath;
}

const void* Image::GetRawData() const
{
	return m_rgbaTexels.data();
}

IntVec2 Image::GetDimensions() const
{
	return m_dimensions;
}

Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const
{
	GUARANTEE_OR_DIE(IsInBounds(texelCoords), "Invalid texelCoords in Image::GetTexelColor");
	int index = texelCoords.x + m_dimensions.x * texelCoords.y;
	return m_rgbaTexels[index];
}

void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{
	GUARANTEE_OR_DIE(IsInBounds(texelCoords), "Invalid texelCoords in Image::SetTexelColor");
	int index = texelCoords.x + m_dimensions.x * texelCoords.y;
	m_rgbaTexels[index] = newColor;
}

bool Image::IsInBounds(IntVec2 const& texelCoords) const
{
	return	(texelCoords.x >= 0) && (texelCoords.x < m_dimensions.x) &&
			(texelCoords.y >= 0) && (texelCoords.y < m_dimensions.y);
}

Image Image::GetBoxBlurred(int blurRadius) const
{
	if (blurRadius < 1)
	{
		return (*this);
	}

	Image result = (*this);

	int numTexelX = m_dimensions.x;
	int numTexelY = m_dimensions.y;

	for (int texelY = 0; texelY < numTexelY; ++texelY)
	{
		for (int texelX = 0; texelX < numTexelX; ++texelX)
		{
			float sumR = 0.f;
			float sumG = 0.f;
			float sumB = 0.f;
			float sumA = 0.f;
			int count = 0;

			for (int ky = -blurRadius; ky <= blurRadius; ++ky)
			{
				for (int kx = -blurRadius; kx <= blurRadius; ++kx)
				{
					int neighborX = texelX + kx;
					int neighborY = texelY + ky;
					IntVec2 neighborCoords(neighborX, neighborY);
					if (IsInBounds(neighborCoords))
					{
						Rgba8 neighborColor = GetTexelColor(neighborCoords);
						sumR += NormalizeByte(neighborColor.r);
						sumG += NormalizeByte(neighborColor.g);
						sumB += NormalizeByte(neighborColor.b);
						sumA += NormalizeByte(neighborColor.a);
						++count;
					}
				}
			}

			result.SetTexelColor(IntVec2(texelX, texelY), Rgba8(DenormalizeByte(sumR / count), DenormalizeByte(sumG / count), DenormalizeByte(sumB / count), DenormalizeByte(sumA / count)));
		}
	}

	return result;
}
