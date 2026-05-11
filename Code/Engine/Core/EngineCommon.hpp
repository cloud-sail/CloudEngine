#pragma once
//-----------------------------------------------------------------------------------------------
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include <cstdint>

//-----------------------------------------------------------------------------------------------
#define UNUSED(x) (void)(x)
#define STATIC

//-----------------------------------------------------------------------------------------------
enum class BufferEndian
{
	NATIVE,
	LITTLE,
	BIG,
};

BufferEndian GetPlatformNativeEndianMode();

// Byte-swap helpers: reverse bytes in-place for endian conversion
void Reverse2BytesInPlace(void* data);
void Reverse4BytesInPlace(void* data);
void Reverse8BytesInPlace(void* data);


//-----------------------------------------------------------------------------------------------
class NamedStrings;
class EventSystem;
class DevConsole;
class InputSystem;

//-----------------------------------------------------------------------------------------------
extern NamedStrings g_gameConfigBlackboard;
extern EventSystem* g_theEventSystem;
extern DevConsole*	g_theDevConsole;
extern InputSystem* g_theInput;
 