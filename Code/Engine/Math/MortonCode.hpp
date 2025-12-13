#pragma once

#include <cstdint>

// zyx zyx zyx zyx zyx

/*
How to use?

uint32_t x1 = 123, y1 = 456, z1 = 789;
uint32_t morton32 = MortonCode10Bit::Encode(x1, y1, z1);

IntVec3 coords;
uint32_t morton32 = MortonCode10Bit::Encode((uint32_t)coords.x, (uint32_t)coords.y, (uint32_t)coords.z);

uint32_t x2, y2, z2;
MortonCode10Bit::Decode(morton32, x2, y2, z2);

IntVec3((int)x2, (int)y2, (int)z2);

*/



class MortonCode
{
public:
	// Encodes 3D coordinates into Morton Code
	static uint64_t Encode(uint32_t x, uint32_t y, uint32_t z) 
	{
		return (splitBy3(z) << 2) | (splitBy3(y) << 1) | splitBy3(x);
	}

	// Decodes Morton Code into 3D coordinates
	static void Decode(uint64_t code, uint32_t& x, uint32_t& y, uint32_t& z) 
	{
		z = compactBy3(code >> 2);
		y = compactBy3(code >> 1);
		x = compactBy3(code);
	}

private:
	// Spreads bits of a single 32-bit integer for Morton encoding
	static uint64_t splitBy3(uint32_t x) 
	{
		uint64_t result = x & 0x1fffff; // Keep only the lower 21 bits (since 21*3=63 bits)

		result = (result | result << 32) & 0x1f00000000ffff;
		result = (result | result << 16) & 0x1f0000ff0000ff;
		result = (result | result << 8) & 0x100f00f00f00f00f;
		result = (result | result << 4) & 0x10c30c30c30c30c3;
		result = (result | result << 2) & 0x1249249249249249;

		return result;
	}

	// Compacts the spread bits back into the original integer
	static uint32_t compactBy3(uint64_t x) 
	{
		x &= 0x1249249249249249;
		x = (x ^ (x >> 2)) & 0x10c30c30c30c30c3;
		x = (x ^ (x >> 4)) & 0x100f00f00f00f00f;
		x = (x ^ (x >> 8)) & 0x1f0000ff0000ff;
		x = (x ^ (x >> 16)) & 0x1f00000000ffff;
		x = (x ^ (x >> 32)) & 0x1fffff;

		return static_cast<uint32_t>(x);
	}

};

// smaller coordinate ranges(e.g., 10 - bit)
// Has Been Proved
class MortonCode10Bit 
{
public:
	// (for each coordinate < 1024)
	static uint32_t Encode(uint32_t x, uint32_t y, uint32_t z) 
	{
		return (splitBy3_10bit(z) << 2) | (splitBy3_10bit(y) << 1) | splitBy3_10bit(x);
	}

	static void Decode(uint32_t code, uint32_t& x, uint32_t& y, uint32_t& z) 
	{
		z = compactBy3_10bit(code >> 2);
		y = compactBy3_10bit(code >> 1);
		x = compactBy3_10bit(code);
	}

private:
	// Spreads bits of a 10-bit integer
	static uint32_t splitBy3_10bit(uint32_t x) 
	{
		x &= 0x000003ff; // Keep the lower 10 bits
		x = (x | x << 16) & 0x030000ff;
		x = (x | x << 8) & 0x0300f00f;
		x = (x | x << 4) & 0x030c30c3;
		x = (x | x << 2) & 0x09249249;
		return x;
	}

	// Compacts the spread bits back
	static uint32_t compactBy3_10bit(uint32_t x) 
	{
		x &= 0x09249249;
		x = (x ^ (x >> 2)) & 0x030c30c3;
		x = (x ^ (x >> 4)) & 0x0300f00f;
		x = (x ^ (x >> 8)) & 0x030000ff;
		x = (x ^ (x >> 16)) & 0x000003ff;
		return x;
	}

};


// ==================== 2D Morton Code ====================

class MortonCode2D 
{
public:
	// Encodes 2D coordinates into Morton Code
	static uint64_t Encode(uint32_t x, uint32_t y) 
	{
		return (splitBy2(y) << 1) | splitBy2(x);
	}

	// Decodes Morton Code into 2D coordinates
	static void Decode(uint64_t code, uint32_t& x, uint32_t& y) 
	{
		y = compactBy2(code >> 1);
		x = compactBy2(code);
	}

private:
	// Spreads bits of a single 32-bit integer (2D version, insert one 0 between each bit)
	static uint64_t splitBy2(uint32_t x) 
	{
		uint64_t result = x & 0xffffffff;

		result = (result | result << 16) & 0x0000ffff0000ffff;
		result = (result | result << 8) & 0x00ff00ff00ff00ff;
		result = (result | result << 4) & 0x0f0f0f0f0f0f0f0f;
		result = (result | result << 2) & 0x3333333333333333;
		result = (result | result << 1) & 0x5555555555555555;

		return result;
	}

	// Compacts the spread bits back into the original integer (2D version)
	static uint32_t compactBy2(uint64_t x) 
	{
		x &= 0x5555555555555555;
		x = (x ^ (x >> 1)) & 0x3333333333333333;
		x = (x ^ (x >> 2)) & 0x0f0f0f0f0f0f0f0f;
		x = (x ^ (x >> 4)) & 0x00ff00ff00ff00ff;
		x = (x ^ (x >> 8)) & 0x0000ffff0000ffff;
		x = (x ^ (x >> 16)) & 0x00000000ffffffff;

		return static_cast<uint32_t>(x);
	}

};

// 16-bit version (each coordinate < 65536)
class MortonCode2D_16Bit {
public:
	// Encodes (for each coordinate < 65536)
	static uint32_t Encode(uint32_t x, uint32_t y) 
	{
		return (splitBy2_16bit(y) << 1) | splitBy2_16bit(x);
	}

	// Decodes
	static void Decode(uint32_t code, uint32_t& x, uint32_t& y) 
	{
		y = compactBy2_16bit(code >> 1);
		x = compactBy2_16bit(code);
	}

private:
	// Spreads bits of a 16-bit integer
	static uint32_t splitBy2_16bit(uint32_t x) 
	{
		x &= 0x0000ffff;
		x = (x | x << 8) & 0x00ff00ff;
		x = (x | x << 4) & 0x0f0f0f0f;
		x = (x | x << 2) & 0x33333333;
		x = (x | x << 1) & 0x55555555;
		return x;
	}

	// Compacts the spread bits back
	static uint32_t compactBy2_16bit(uint32_t x) 
	{
		x &= 0x55555555;
		x = (x ^ (x >> 1)) & 0x33333333;
		x = (x ^ (x >> 2)) & 0x0f0f0f0f;
		x = (x ^ (x >> 4)) & 0x00ff00ff;
		x = (x ^ (x >> 8)) & 0x0000ffff;
		return x;
	}

};
