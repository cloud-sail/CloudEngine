#include "Engine/Core/BufferParser.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/EulerAngles.hpp"

BufferParser::BufferParser(void const* data, size_t size)
	: m_data(static_cast<unsigned char const*>(data))
	, m_size(size)
	, m_currentPos(0)
{
}

BufferParser::BufferParser(std::vector<unsigned char> const& buffer)
	: m_data(buffer.data())
	, m_size(buffer.size())
	, m_currentPos(0)
{
}

void BufferParser::SetEndianMode(BufferEndian endianMode)
{
	m_endianMode = endianMode;
}

bool BufferParser::IsOppositeEndian() const
{
	if (m_endianMode == BufferEndian::NATIVE)
		return false;
	return m_endianMode != GetPlatformNativeEndianMode();
}

void BufferParser::GuardRead(size_t numBytes) const
{
	GUARANTEE_RECOVERABLE(m_currentPos + numBytes <= m_size,
		"BufferParser: attempted to read beyond end of buffer!");
}

unsigned char BufferParser::ParseByte()
{
	GuardRead(1);
	return m_data[m_currentPos++];
}

char BufferParser::ParseChar()
{
	GuardRead(1);
	return static_cast<char>(m_data[m_currentPos++]);
}

bool BufferParser::ParseBool()
{
	GuardRead(1);
	return m_data[m_currentPos++] != 0;
}

unsigned short BufferParser::ParseUshort() { return ParseBytes<unsigned short>(); }
short          BufferParser::ParseShort() { return ParseBytes<short>(); }
uint32_t       BufferParser::ParseUint32() { return ParseBytes<uint32_t>(); }
int32_t        BufferParser::ParseInt32() { return ParseBytes<int32_t>(); }
uint64_t       BufferParser::ParseUint64() { return ParseBytes<uint64_t>(); }
int64_t        BufferParser::ParseInt64() { return ParseBytes<int64_t>(); }
float          BufferParser::ParseFloat() { return ParseBytes<float>(); }
double         BufferParser::ParseDouble() { return ParseBytes<double>(); }

void BufferParser::ParseStringZeroTerminated(std::string& out_string)
{
	out_string.clear();
	while (m_currentPos < m_size)
	{
		unsigned char c = m_data[m_currentPos++];
		if (c == 0)
			return;
		out_string += static_cast<char>(c);
	}
}

void BufferParser::ParseStringAfter32BitLength(std::string& out_string)
{
	uint32_t length = ParseUint32();
	GuardRead(length);
	out_string.clear();
	out_string.assign(reinterpret_cast<char const*>(&m_data[m_currentPos]), length);
	m_currentPos += length;
}

Rgba8 BufferParser::ParseRgba()
{
	unsigned char r = ParseByte();
	unsigned char g = ParseByte();
	unsigned char b = ParseByte();
	unsigned char a = ParseByte();
	return Rgba8(r, g, b, a);
}

Rgba8 BufferParser::ParseRgb()
{
	unsigned char r = ParseByte();
	unsigned char g = ParseByte();
	unsigned char b = ParseByte();
	return Rgba8(r, g, b, 255);
}

IntVec2 BufferParser::ParseIntVec2()
{
	int x = ParseInt32();
	int y = ParseInt32();
	return IntVec2(x, y);
}

Vec2 BufferParser::ParseVec2()
{
	float x = ParseFloat();
	float y = ParseFloat();
	return Vec2(x, y);
}

Vec3 BufferParser::ParseVec3()
{
	float x = ParseFloat();
	float y = ParseFloat();
	float z = ParseFloat();
	return Vec3(x, y, z);
}

Vertex_PCU BufferParser::ParseVertexPCU()
{
	Vec3 pos = ParseVec3();
	Rgba8 color = ParseRgba();
	Vec2 uv = ParseVec2();
	return Vertex_PCU(pos, color, uv);
}


EulerAngles BufferParser::ParseEulerAngles()
{
	float yawDegrees = ParseFloat();
	float pitchDegrees = ParseFloat();
	float rollDegrees = ParseFloat();
	return EulerAngles(yawDegrees, pitchDegrees, rollDegrees);
}

void BufferParser::SetReadPosition(size_t position)
{
	m_currentPos = position;
}
