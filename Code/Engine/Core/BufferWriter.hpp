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
class BufferWriter
{
public:
	BufferWriter(std::vector<unsigned char>& buffer);

	void SetEndianMode( BufferEndian endianMode );
	BufferEndian GetEndianMode() const { return m_endianMode; }

	// Primitive type appends
	void AppendByte(unsigned char value);
	void AppendChar(char value);
	void AppendBool(bool value);
	void AppendUshort(unsigned short value);
	void AppendShort(short value);
	void AppendUint32(uint32_t value);
	void AppendInt32(int32_t value);
	void AppendUint64(uint64_t value);
	void AppendInt64(int64_t value);
	void AppendFloat(float value);
	void AppendDouble(double value);

	// String appends
	void AppendStringZeroTerminated( std::string const& str );
	void AppendStringAfter32BitLength( std::string const& str );

	// Engine semi-primitive type appends
	void AppendRgba( Rgba8 const& rgba );
	void AppendRgb( Rgba8 const& rgb );
	void AppendIntVec2(IntVec2 const& iv2);
	void AppendIntVec3(IntVec3 const& iv3);
	void AppendVec2(Vec2 const& v2);
	void AppendVec3(Vec3 const& v3);
	void AppendVec4(Vec4 const& v4);
	void AppendVertexPCU(Vertex_PCU const& vertex);
	void AppendEulerAngles(EulerAngles const& orientation);

	// Random-access overwrite
	void OverwriteUint32(size_t position, uint32_t value);

	int GetTotalSize() const { return (int)m_buffer.size(); }

private:
	bool IsOppositeEndian() const;

private:
	template <typename T>
	void AppendBytes(T value)
	{
		unsigned char bytes[sizeof(T)];
		memcpy(bytes, &value, sizeof(T));
		if (IsOppositeEndian())
		{
			if constexpr (sizeof(T) == 2) Reverse2BytesInPlace(bytes);
			else if constexpr (sizeof(T) == 4) Reverse4BytesInPlace(bytes);
			else if constexpr (sizeof(T) == 8) Reverse8BytesInPlace(bytes);
		}
		for (size_t i = 0; i < sizeof(T); ++i)
			m_buffer.push_back(bytes[i]);
	}

private:
	std::vector<unsigned char>& m_buffer;
	BufferEndian m_endianMode = BufferEndian::NATIVE;
};
