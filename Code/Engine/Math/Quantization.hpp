#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"

#include <cstdint>
#include <algorithm>
#include <cmath>


namespace Quantization
{
	inline float GetFloatClamped(float value, float minValue, float maxValue)
	{
		if (value < minValue) return minValue;
		if (value > maxValue) return maxValue;
		return value;
	}

	inline float ToUNormFromUint8(uint8_t b)
	{
		return static_cast<float>(b) / 255.f;
	}

	inline uint8_t ToUint8FromUNorm(float zeroToOne)
	{
		return static_cast<uint8_t>(GetFloatClamped(zeroToOne * 256.f, 0.f, 255.f));
	}

	inline uint8_t ToUint8FromSNorm(float minusOneToOne)
	{
		float zeroToOne = minusOneToOne * 0.5f + 0.5f;
		return ToUint8FromUNorm(zeroToOne);
	}

	// Octahedron Wrap
	inline void OctWrap(float& x, float& y)
	{
		float signX = x >= 0.0f ? 1.0f : -1.0f;
		float signY = y >= 0.0f ? 1.0f : -1.0f;
		float tempX = (1.0f - std::abs(y)) * signX;
		float tempY = (1.0f - std::abs(x)) * signY;
		x = tempX;
		y = tempY;
	}

	// encode Vec3 normal to 2 uint8_t
	inline void OctEncodeNormal(const Vec3& normal, uint8_t& outX, uint8_t& outY)
	{
		Vec3 n = normal.GetNormalized();

		float absSum = std::abs(n.x) + std::abs(n.y) + std::abs(n.z);
		float nx = n.x / absSum;
		float ny = n.y / absSum;
		float nz = n.z / absSum;


		if (nz < 0.0f)
		{
			OctWrap(nx, ny);
		}

		outX = ToUint8FromSNorm(nx);
		outY = ToUint8FromSNorm(ny);
	}

	inline Vec2 OctEncodeNormal(Vec3 const& normal)
	{
		Vec3 n = normal.GetNormalized();

		float absSum = std::abs(n.x) + std::abs(n.y) + std::abs(n.z);
		float nx = n.x / absSum;
		float ny = n.y / absSum;
		float nz = n.z / absSum;


		if (nz < 0.0f) 
		{
			OctWrap(nx, ny);
		}

		Vec2 result;
		result.x = nx;
		result.y = ny;
		return result;
	}

};

