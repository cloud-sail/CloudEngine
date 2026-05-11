#include "Engine/Core/BufferWriter.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include <cstring>

BufferWriter::BufferWriter(std::vector<unsigned char>& buffer)
	: m_buffer(buffer)
{
}

void BufferWriter::SetEndianMode(BufferEndian endianMode)
{
	m_endianMode = endianMode;
}

bool BufferWriter::IsOppositeEndian() const
{
	if (m_endianMode == BufferEndian::NATIVE)
		return false;
	return m_endianMode != GetPlatformNativeEndianMode();
}

void BufferWriter::AppendByte(unsigned char value)
{
	m_buffer.push_back(value);
}

void BufferWriter::AppendChar(char value)
{
	m_buffer.push_back(static_cast<unsigned char>(value));
}

void BufferWriter::AppendBool(bool value)
{
	m_buffer.push_back(value ? 1 : 0);
}

void BufferWriter::AppendUshort(unsigned short value)
{
	AppendBytes(value);
}

void BufferWriter::AppendShort(short value)
{
	AppendBytes(value);
}

void BufferWriter::AppendUint32(uint32_t value)
{
	AppendBytes(value);
}

void BufferWriter::AppendInt32(int32_t value)
{
	AppendBytes(value);
}

void BufferWriter::AppendUint64(uint64_t value)
{
	AppendBytes(value);
}

void BufferWriter::AppendInt64(int64_t value)
{
	AppendBytes(value);
}

void BufferWriter::AppendFloat(float value)
{
	AppendBytes(value);
}

void BufferWriter::AppendDouble(double value)
{
	AppendBytes(value);
}

void BufferWriter::AppendStringZeroTerminated(std::string const& str)
{
	for (size_t i = 0; i < str.length(); ++i)
	{
		AppendChar(str[i]);
	}
	AppendByte(0); 
}

void BufferWriter::AppendStringAfter32BitLength(std::string const& str)
{
	AppendUint32((uint32_t)str.length());
	for (size_t i = 0; i < str.length(); ++i)
	{
		AppendChar(str[i]);
	}
}

void BufferWriter::AppendRgba(Rgba8 const& rgba)
{
	AppendByte(rgba.r);
	AppendByte(rgba.g);
	AppendByte(rgba.b);
	AppendByte(rgba.a);
}

void BufferWriter::AppendRgb(Rgba8 const& rgb)
{
	AppendByte(rgb.r);
	AppendByte(rgb.g);
	AppendByte(rgb.b);
}

void BufferWriter::AppendIntVec2(IntVec2 const& iv2)
{
	AppendInt32(iv2.x);
	AppendInt32(iv2.y);
}


void BufferWriter::AppendIntVec3(IntVec3 const& iv3)
{
	AppendInt32(iv3.x);
	AppendInt32(iv3.y);
	AppendInt32(iv3.z);
}

void BufferWriter::AppendVec2(Vec2 const& v2)
{
	AppendFloat(v2.x);
	AppendFloat(v2.y);
}

void BufferWriter::AppendVec3(Vec3 const& v3)
{
	AppendFloat(v3.x);
	AppendFloat(v3.y);
	AppendFloat(v3.z);
}


void BufferWriter::AppendVec4(Vec4 const& v4)
{
	AppendFloat(v4.x);
	AppendFloat(v4.y);
	AppendFloat(v4.z);
	AppendFloat(v4.w);
}

void BufferWriter::AppendVertexPCU(Vertex_PCU const& vertex)
{
	AppendVec3(vertex.m_position);
	AppendRgba(vertex.m_color);
	AppendVec2(vertex.m_uvTexCoords);
}


void BufferWriter::AppendEulerAngles(EulerAngles const& orientation)
{
	AppendFloat(orientation.m_yawDegrees);
	AppendFloat(orientation.m_pitchDegrees);
	AppendFloat(orientation.m_rollDegrees);
}

void BufferWriter::OverwriteUint32(size_t position, uint32_t value)
{
	unsigned char bytes[4];
	memcpy(bytes, &value, 4);
	if (IsOppositeEndian())
		Reverse4BytesInPlace(bytes);
	m_buffer[position] = bytes[0];
	m_buffer[position + 1] = bytes[1];
	m_buffer[position + 2] = bytes[2];
	m_buffer[position + 3] = bytes[3];
}
