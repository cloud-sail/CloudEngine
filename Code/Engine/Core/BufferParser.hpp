#pragma once
#include "Engine/Core/EngineCommon.hpp"

#include <string>
#include <vector>

//-----------------------------------------------------------------------------------------------
struct Rgba8;
struct IntVec2;
struct IntVec3;
struct Vec2;
struct Vec3;
struct Vec4;
struct Vertex_PCU;
struct EulerAngles;

//-----------------------------------------------------------------------------------------------
class BufferParser
{
public:
	// Construct from raw data pointer + size
	BufferParser(void const* data, size_t size);

	// Construct from a const reference to a byte vector
	BufferParser(std::vector<unsigned char> const& buffer);

	void SetEndianMode(BufferEndian endianMode);
	BufferEndian GetEndianMode() const { return m_endianMode; }

	// Primitive type parsing (reads and advances position)
	unsigned char	ParseByte();
	char			ParseChar();
	bool			ParseBool();
	unsigned short	ParseUshort();
	short			ParseShort();
	uint32_t		ParseUint32();
	int32_t			ParseInt32();
	uint64_t		ParseUint64();
	int64_t			ParseInt64();
	float			ParseFloat();
	double			ParseDouble();

	// String parsing
	void ParseStringZeroTerminated(std::string& out_string);
	void ParseStringAfter32BitLength(std::string& out_string);

	// Engine semi-primitive type parsing
	Rgba8		ParseRgba();
	Rgba8		ParseRgb();
	IntVec2		ParseIntVec2();
	Vec2		ParseVec2();
	Vec3		ParseVec3();
	Vertex_PCU	ParseVertexPCU();
	EulerAngles	ParseEulerAngles();

	// Seek to absolute offset
	void SetReadPosition(size_t position);
	size_t GetReadPosition() const { return m_currentPos; }

	size_t GetRemainingSize() const { return m_size - m_currentPos; }
	size_t GetTotalSize() const { return m_size; }

private:
	bool IsOppositeEndian() const;
	void GuardRead(size_t numBytes) const;


private:
	template <typename T>
	T ParseBytes()
	{
		GuardRead(sizeof(T));
		unsigned char bytes[sizeof(T)];
		memcpy(bytes, &m_data[m_currentPos], sizeof(T));
		m_currentPos += sizeof(T);
		if (IsOppositeEndian())
		{
			if constexpr (sizeof(T) == 2) Reverse2BytesInPlace(bytes);
			else if constexpr (sizeof(T) == 4) Reverse4BytesInPlace(bytes);
			else if constexpr (sizeof(T) == 8) Reverse8BytesInPlace(bytes);
		}
		T value;
		memcpy(&value, bytes, sizeof(T));
		return value;
	}

private:
	unsigned char const* m_data = nullptr;
	size_t m_size = 0;
	size_t m_currentPos = 0;
	BufferEndian m_endianMode = BufferEndian::NATIVE;
};
