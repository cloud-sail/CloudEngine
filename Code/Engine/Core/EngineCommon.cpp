#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/NamedStrings.hpp"

NamedStrings g_gameConfigBlackboard;

BufferEndian GetPlatformNativeEndianMode()
{
	uint32_t testValue = 1;
	uint8_t* bytes = reinterpret_cast<uint8_t*>( &testValue );
	if ( bytes[0] == 1 )
	{
		return BufferEndian::LITTLE;
	}
	else
	{
		return BufferEndian::BIG;
	}
}

void Reverse2BytesInPlace(void* data)
{
	uint8_t* bytes = static_cast<uint8_t*>(data);
	std::swap(bytes[0], bytes[1]);
}

void Reverse4BytesInPlace(void* data)
{
	uint8_t* bytes = static_cast<uint8_t*>(data);
	std::swap(bytes[0], bytes[3]);
	std::swap(bytes[1], bytes[2]);
}

void Reverse8BytesInPlace(void* data)
{
	uint8_t* bytes = static_cast<uint8_t*>(data);
	std::swap(bytes[0], bytes[7]);
	std::swap(bytes[1], bytes[6]);
	std::swap(bytes[2], bytes[5]);
	std::swap(bytes[3], bytes[4]);
}
